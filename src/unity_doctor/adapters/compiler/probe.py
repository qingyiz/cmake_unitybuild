from __future__ import annotations

import hashlib
import json
import os
import re
import shlex
import subprocess
from pathlib import Path
from typing import Dict, List, Optional, Sequence

from unity_doctor.adapters.process import SubprocessExecutor
from unity_doctor.domain.models import (
    FailureFingerprint,
    ProbeRecord,
)


_DIAGNOSTIC = re.compile(
    r"^(?P<path>.+?):(?P<line>\d+)(?::(?P<column>\d+))?:\s+"
    r"(?P<level>fatal error|error):\s+(?P<message>.+)$"
)


def parse_failure_fingerprint(
    output: str, compiler_family: str = "unknown", phase: str = "compile"
) -> Optional[FailureFingerprint]:
    for line in output.splitlines():
        match = _DIAGNOSTIC.match(line.strip())
        if not match:
            continue
        message = re.sub(r"\s+\[-W[^\]]+\]$", "", match.group("message")).strip()
        category = _diagnostic_category(message)
        symbol_match = re.search(r"['‘“`]([^'’”`]+)['’”`]", message)
        symbol = symbol_match.group(1) if symbol_match else ""
        normalized = re.sub(r"\b\d+\b", "#", message)
        return FailureFingerprint(
            compiler_family=compiler_family,
            phase=phase,
            category=category,
            symbol=symbol,
            message=normalized,
        )
    lowered = output.lower()
    if "error " in lowered and "c" in compiler_family:
        message = next(
            (line.strip() for line in output.splitlines() if "error " in line.lower()),
            "compiler error",
        )
        return FailureFingerprint(
            compiler_family, phase, _diagnostic_category(message), "", message
        )
    return None


def detect_compiler_family(entry: Dict[str, object]) -> str:
    raw_arguments = entry.get("arguments")
    arguments = (
        [str(item) for item in raw_arguments]
        if isinstance(raw_arguments, list)
        else shlex.split(str(entry.get("command", "")), posix=os.name != "nt")
    )
    direct = _compiler_family(arguments)
    if direct in {"clang", "msvc"}:
        return direct
    for candidate in arguments[:3]:
        if candidate.startswith("-"):
            continue
        try:
            result = subprocess.run(
                [candidate, "--version"],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                errors="replace",
                timeout=5,
                check=False,
            )
        except (OSError, subprocess.SubprocessError):
            continue
        lowered = result.stdout.lower()
        if "clang" in lowered:
            return "clang"
        if "gcc" in lowered or "free software foundation" in lowered:
            return "gcc"
        if "microsoft" in lowered:
            return "msvc"
    return direct


class CompilerProbeRunner:
    def __init__(
        self,
        probe_root: Path,
        timeout_seconds: float = 300.0,
        executor: Optional[SubprocessExecutor] = None,
    ) -> None:
        self.probe_root = probe_root.resolve()
        self.timeout_seconds = timeout_seconds
        self.executor = executor or SubprocessExecutor()

    def run(
        self,
        sources: Sequence[str],
        compile_entry: Dict[str, object],
        expected: FailureFingerprint,
        cache: Optional[Dict[str, ProbeRecord]] = None,
    ) -> ProbeRecord:
        source_paths = [Path(item).resolve() for item in sources]
        key = _probe_key(source_paths, compile_entry, expected.key)
        if cache and key in cache:
            return cache[key]
        probe_dir = self.probe_root / key[:16]
        probe_dir.mkdir(parents=True, exist_ok=True)
        suffix = ".c" if any(path.suffix == ".c" for path in source_paths) else ".cxx"
        driver = probe_dir / ("probe" + suffix)
        driver.write_text(
            "\n".join('#include "{}"'.format(_escape_include(path)) for path in source_paths)
            + "\n",
            encoding="utf-8",
        )
        output_object = probe_dir / "probe.o"
        argv = _rewrite_compile_command(compile_entry, driver, output_object)
        command = self.executor.run(
            argv,
            Path(str(compile_entry.get("directory", probe_dir))),
            probe_dir / "compile.log",
            self.timeout_seconds,
        )
        text = Path(command.log_path).read_text(encoding="utf-8", errors="replace")
        detected_family = _compiler_family(argv)
        family = (
            expected.compiler_family
            if expected.compiler_family != "unknown"
            else detected_family
        )
        fingerprint = parse_failure_fingerprint(text, family)
        reproduced = bool(fingerprint and fingerprint.key == expected.key)
        return ProbeRecord(key, [str(path) for path in source_paths], command, reproduced, fingerprint)


def _rewrite_compile_command(
    entry: Dict[str, object], driver: Path, output_object: Path
) -> List[str]:
    raw_arguments = entry.get("arguments")
    if isinstance(raw_arguments, list):
        arguments = [str(item) for item in raw_arguments]
    else:
        command = entry.get("command")
        if not isinstance(command, str):
            raise ValueError("compile_commands entry 缺少 arguments/command")
        arguments = shlex.split(command, posix=os.name != "nt")
    if any(item.startswith("@") for item in arguments):
        raise ValueError("MVP 不支持包含 response file 的编译命令")
    original = Path(str(entry.get("file", ""))).resolve()
    rewritten: List[str] = []
    skip_next = False
    pair_flags = {"-o", "-MF", "-MT", "-MQ", "/Fo"}
    standalone_remove = {"-MD", "-MMD", "-MP", "-MJ"}
    for item in arguments:
        if skip_next:
            skip_next = False
            continue
        if item in pair_flags:
            skip_next = True
            continue
        if item in standalone_remove:
            continue
        if item.startswith("/Fo") and item != "/Fo":
            continue
        try:
            if Path(item).resolve() == original:
                continue
        except (OSError, ValueError):
            pass
        rewritten.append(item)
    if os.name == "nt" and any(Path(item).name.lower() == "cl.exe" for item in rewritten):
        rewritten.extend(["/c", str(driver), "/Fo{}".format(output_object)])
    else:
        if "-c" not in rewritten:
            rewritten.append("-c")
        rewritten.extend([str(driver), "-o", str(output_object)])
    return rewritten


def _compiler_family(argv: Sequence[str]) -> str:
    joined = " ".join(Path(item).name.lower() for item in argv[:3])
    if "clang" in joined:
        return "clang"
    if "cl.exe" in joined or re.search(r"(^|\s)cl(\s|$)", joined):
        return "msvc"
    if "gcc" in joined or "g++" in joined or "c++" in joined or "cc" in joined:
        return "gcc"
    return "unknown"


def _diagnostic_category(message: str) -> str:
    lowered = message.lower()
    if "redefinition" in lowered or "already defined" in lowered:
        return "redefinition"
    if "macro" in lowered:
        return "macro"
    if "undeclared" in lowered or "not declared" in lowered or "unknown type" in lowered:
        return "missing_declaration"
    if "incomplete type" in lowered:
        return "incomplete_type"
    if "file not found" in lowered or "no such file" in lowered:
        return "missing_include"
    if "static assertion" in lowered or "#error" in lowered:
        return "preprocessor"
    return "compiler_error"


def _probe_key(
    sources: Sequence[Path], entry: Dict[str, object], expected_key: str
) -> str:
    digest = hashlib.sha256()
    for source in sources:
        digest.update(str(source).encode())
        digest.update(source.read_bytes())
    compile_data = {
        "arguments": entry.get("arguments"),
        "command": entry.get("command"),
        "directory": entry.get("directory"),
    }
    digest.update(json.dumps(compile_data, sort_keys=True).encode())
    digest.update(expected_key.encode())
    return digest.hexdigest()


def _escape_include(path: Path) -> str:
    return str(path).replace("\\", "\\\\").replace('"', '\\"')
