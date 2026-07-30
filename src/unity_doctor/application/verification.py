from __future__ import annotations

import shlex
import statistics
from pathlib import Path
from typing import Any, Callable

from unity_doctor.application.diagnosis import new_session_id
from unity_doctor.application.requests import DiagnosisRequest, VerificationRequest
from unity_doctor.domain.models import Session, SessionStatus
from unity_doctor.timeutil import utc_now


class VerificationService:
    def __init__(
        self,
        cmake: Any,
        executor: Any,
        store_factory: Callable[[Path], Any],
        reporter_factory: Callable[[Path], Any],
    ) -> None:
        self.cmake = cmake
        self.executor = executor
        self.store_factory = store_factory
        self.reporter_factory = reporter_factory

    def verify(self, raw: VerificationRequest) -> Session:
        normalized = DiagnosisRequest(
            source=raw.source,
            work_dir=raw.work_dir,
            generator=raw.generator,
            config=raw.config,
            targets=raw.targets,
            cmake_args=raw.cmake_args,
            timeout=raw.timeout,
        ).normalized()
        session_id = new_session_id()
        report_root = Path(normalized.report_dir)
        session = Session(
            session_id,
            utc_now(),
            utc_now(),
            SessionStatus.INSPECTING,
            {
                **normalized.__dict__,
                "test_command": raw.test_command,
                "benchmark_runs": raw.benchmark_runs,
            },
        )
        build_root = Path(normalized.work_dir) / "verify" / session_id
        results = {}
        for mode, enabled in (("baseline", False), ("unity", True)):
            outcome = self.cmake.configure_and_build(
                Path(normalized.source),
                build_root / mode,
                enabled,
                normalized.generator or None,
                normalized.config,
                normalized.targets,
                normalized.cmake_args,
                normalized.timeout,
            )
            session.commands.extend(
                item for item in (outcome.configure, outcome.build) if item is not None
            )
            results[mode] = {
                "passed": outcome.succeeded,
                "stage": outcome.failed_stage,
            }
        if raw.test_command:
            command = self.executor.run(
                shlex.split(raw.test_command),
                Path(normalized.source),
                build_root / "tests" / "test.log",
                normalized.timeout,
            )
            session.commands.append(command)
            results["tests"] = {"passed": command.exit_code == 0}
        if raw.benchmark_runs > 0 and results["unity"]["passed"]:
            durations = []
            for index in range(raw.benchmark_runs):
                records = self.cmake.clean_and_build(
                    build_root / "unity",
                    normalized.config,
                    normalized.targets,
                    build_root / "benchmark" / str(index + 1),
                    normalized.timeout,
                )
                session.commands.extend(records)
                if len(records) == 2 and records[-1].exit_code == 0:
                    durations.append(records[-1].duration_seconds)
            benchmark = {"runs": durations}
            if len(durations) >= 3:
                benchmark["median_seconds"] = statistics.median(durations)
            results["benchmark"] = benchmark
        correctness = results["baseline"]["passed"] and results["unity"]["passed"]
        if "tests" in results:
            correctness = correctness and results["tests"]["passed"]
        results["verified"] = correctness
        session.verification = results
        session.status = (
            SessionStatus.VERIFIED if correctness else SessionStatus.VERIFY_FAILED
        )
        self.store_factory(report_root).save(session)
        self.reporter_factory(report_root).render(session)
        return session
