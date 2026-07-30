from __future__ import annotations

import json
import re
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

from unity_doctor.adapters.process import SubprocessExecutor
from unity_doctor.domain.models import CommandRecord


@dataclass(frozen=True)
class BuildOutcome:
    mode: str
    build_dir: Path
    configure: CommandRecord
    build: Optional[CommandRecord]

    @property
    def succeeded(self) -> bool:
        return self.configure.exit_code == 0 and bool(
            self.build and self.build.exit_code == 0
        )

    @property
    def failed_stage(self) -> str:
        if self.configure.exit_code != 0:
            return "configure"
        if self.build and self.build.exit_code != 0:
            return "build"
        return ""

    def log_text(self) -> str:
        paths = [self.configure.log_path]
        if self.build:
            paths.append(self.build.log_path)
        return "\n".join(
            Path(path).read_text(encoding="utf-8", errors="replace")
            for path in paths
            if Path(path).exists()
        )


@dataclass(frozen=True)
class UnityUnit:
    target: str
    language: str
    unity_source: Path
    ordered_sources: List[Path]
    compile_entry: Dict[str, object]


class CMakeAdapter:
    def __init__(
        self,
        cmake: Optional[str] = None,
        executor: Optional[SubprocessExecutor] = None,
    ) -> None:
        self.cmake = cmake or shutil.which("cmake") or "cmake"
        self.executor = executor or SubprocessExecutor()

    def version(self) -> Tuple[int, int, int]:
        import subprocess

        result = subprocess.run(
            [self.cmake, "--version"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            errors="replace",
            check=False,
        )
        match = re.search(r"cmake version (\d+)\.(\d+)\.(\d+)", result.stdout)
        return tuple(int(item) for item in match.groups()) if match else (0, 0, 0)

    def configure_and_build(
        self,
        source: Path,
        build_dir: Path,
        unity: bool,
        generator: Optional[str],
        config: str,
        targets: Sequence[str],
        cmake_args: Sequence[str],
        timeout_seconds: float,
    ) -> BuildOutcome:
        source = source.resolve()
        build_dir = build_dir.resolve()
        _assert_separate_trees(source, build_dir)
        build_dir.mkdir(parents=True, exist_ok=True)
        self._write_file_api_query(build_dir)
        mode = "unity" if unity else "baseline"
        configure_argv = [
            self.cmake,
            "-S",
            str(source),
            "-B",
            str(build_dir),
            "-DCMAKE_UNITY_BUILD={}".format("ON" if unity else "OFF"),
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        ]
        if generator:
            configure_argv.extend(["-G", generator])
        configure_argv.extend(cmake_args)
        configure = self.executor.run(
            configure_argv,
            source,
            build_dir / "logs" / "configure.log",
            timeout_seconds,
        )
        if configure.exit_code != 0:
            return BuildOutcome(mode, build_dir, configure, None)
        build_argv = [self.cmake, "--build", str(build_dir), "--config", config]
        if targets:
            build_argv.append("--target")
            build_argv.extend(targets)
        build = self.executor.run(
            build_argv,
            source,
            build_dir / "logs" / "build.log",
            timeout_seconds,
        )
        return BuildOutcome(mode, build_dir, configure, build)

    def clean_and_build(
        self,
        build_dir: Path,
        config: str,
        targets: Sequence[str],
        log_dir: Path,
        timeout_seconds: float,
    ) -> List[CommandRecord]:
        records: List[CommandRecord] = []
        clean = [self.cmake, "--build", str(build_dir), "--config", config, "--target", "clean"]
        records.append(
            self.executor.run(clean, build_dir, log_dir / "clean.log", timeout_seconds)
        )
        if records[-1].exit_code != 0:
            return records
        build = [self.cmake, "--build", str(build_dir), "--config", config]
        if targets:
            build.append("--target")
            build.extend(targets)
        records.append(
            self.executor.run(build, build_dir, log_dir / "build.log", timeout_seconds)
        )
        return records

    def discover_unity_units(self, build_dir: Path) -> List[UnityUnit]:
        compile_db = build_dir / "compile_commands.json"
        if not compile_db.exists():
            return []
        entries = json.loads(compile_db.read_text(encoding="utf-8"))
        units: List[UnityUnit] = []
        for entry in entries:
            raw_file = entry.get("file")
            if not isinstance(raw_file, str):
                continue
            unity_source = Path(raw_file)
            if not unity_source.is_absolute():
                unity_source = Path(str(entry.get("directory", build_dir))) / unity_source
            unity_source = unity_source.resolve()
            if "/Unity/" not in unity_source.as_posix() or not unity_source.exists():
                continue
            sources = _parse_unity_includes(unity_source)
            if not sources:
                continue
            units.append(
                UnityUnit(
                    target=_target_from_unity_path(unity_source),
                    language=_language_from_path(unity_source),
                    unity_source=unity_source,
                    ordered_sources=sources,
                    compile_entry=dict(entry),
                )
            )
        return units

    @staticmethod
    def read_codemodel(build_dir: Path) -> Dict[str, object]:
        reply = build_dir / ".cmake" / "api" / "v1" / "reply"
        indexes = sorted(reply.glob("index-*.json"))
        if not indexes:
            return {}
        index = json.loads(indexes[-1].read_text(encoding="utf-8"))
        for item in index.get("objects", []):
            if item.get("kind") == "codemodel":
                path = reply / item["jsonFile"]
                return json.loads(path.read_text(encoding="utf-8"))
        return {}

    @staticmethod
    def _write_file_api_query(build_dir: Path) -> None:
        query = build_dir / ".cmake" / "api" / "v1" / "query" / "client-unity-build-doctor"
        query.mkdir(parents=True, exist_ok=True)
        (query / "codemodel-v2").touch()


def _assert_separate_trees(source: Path, build_dir: Path) -> None:
    if _is_relative_to(build_dir, source) or _is_relative_to(source, build_dir):
        raise ValueError(
            "源码目录与诊断 build 目录不能重叠：source={} build={}".format(
                source, build_dir
            )
        )


def _is_relative_to(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def _parse_unity_includes(unity_source: Path) -> List[Path]:
    includes: List[Path] = []
    pattern = re.compile(r'^\s*#\s*include\s+["<]([^">]+)[">]')
    for line in unity_source.read_text(encoding="utf-8", errors="replace").splitlines():
        match = pattern.match(line)
        if not match:
            continue
        candidate = Path(match.group(1))
        if not candidate.is_absolute():
            candidate = unity_source.parent / candidate
        candidate = candidate.resolve()
        if candidate.suffix.lower() in {".c", ".cc", ".cpp", ".cxx", ".m", ".mm"}:
            includes.append(candidate)
    return includes


def _target_from_unity_path(path: Path) -> str:
    match = re.search(r"/CMakeFiles/(.+?)\.dir/Unity/", path.as_posix())
    return match.group(1) if match else "unknown"


def _language_from_path(path: Path) -> str:
    name = path.name.lower()
    return "C" if "_c.c" in name or name.endswith("_c.c") else "CXX"
