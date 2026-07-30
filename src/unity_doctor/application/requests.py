from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import List


@dataclass(frozen=True)
class DiagnosisRequest:
    source: str
    work_dir: str
    report_dir: str = ""
    generator: str = ""
    config: str = "Debug"
    targets: List[str] = field(default_factory=list)
    cmake_args: List[str] = field(default_factory=list)
    max_probes: int = 100
    timeout: float = 300.0

    def normalized(self) -> "DiagnosisRequest":
        source = str(Path(self.source).expanduser().resolve())
        work_dir = str(Path(self.work_dir).expanduser().resolve())
        report_dir = str(
            Path(self.report_dir).expanduser().resolve()
            if self.report_dir
            else (Path(work_dir) / "reports").resolve()
        )
        request = DiagnosisRequest(
            source=source,
            work_dir=work_dir,
            report_dir=report_dir,
            generator=self.generator,
            config=self.config,
            targets=list(self.targets),
            cmake_args=list(self.cmake_args),
            max_probes=self.max_probes,
            timeout=self.timeout,
        )
        request.validate()
        return request

    def validate(self) -> None:
        source = Path(self.source)
        work = Path(self.work_dir)
        report = Path(self.report_dir) if self.report_dir else work / "reports"
        if not (source / "CMakeLists.txt").is_file():
            raise ValueError("源码目录缺少 CMakeLists.txt：{}".format(source))
        for destination in (work, report):
            if _overlaps(source, destination):
                raise ValueError(
                    "源码目录与诊断输出目录不能重叠：source={} output={}".format(
                        source, destination
                    )
                )
        if self.max_probes < 1:
            raise ValueError("--max-probes 必须大于 0")
        if self.timeout <= 0:
            raise ValueError("--timeout 必须大于 0")
        if any("CMAKE_UNITY_BUILD" in item for item in self.cmake_args):
            raise ValueError("--cmake-arg 不得覆盖 CMAKE_UNITY_BUILD")


@dataclass(frozen=True)
class VerificationRequest:
    source: str
    work_dir: str
    generator: str = ""
    config: str = "Debug"
    targets: List[str] = field(default_factory=list)
    cmake_args: List[str] = field(default_factory=list)
    test_command: str = ""
    benchmark_runs: int = 0
    timeout: float = 300.0


def _overlaps(first: Path, second: Path) -> bool:
    first = first.resolve()
    second = second.resolve()
    try:
        first.relative_to(second)
        return True
    except ValueError:
        pass
    try:
        second.relative_to(first)
        return True
    except ValueError:
        return False
