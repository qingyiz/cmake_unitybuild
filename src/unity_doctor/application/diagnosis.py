from __future__ import annotations

import uuid
from dataclasses import asdict
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Sequence

from unity_doctor.application.requests import DiagnosisRequest
from unity_doctor.domain.diagnostics import build_suggestions, classify_case
from unity_doctor.domain.minimization import minimize_ordered
from unity_doctor.domain.models import (
    ConflictCase,
    FailureFingerprint,
    ProbeRecord,
    Session,
    SessionStatus,
)
from unity_doctor.timeutil import utc_now


class DiagnosisService:
    """Application state machine; all external behavior is injected."""

    def __init__(
        self,
        cmake: Any,
        store_factory: Callable[[Path], Any],
        reporter_factory: Callable[[Path], Any],
        probe_factory: Callable[[Path, float], Any],
        fingerprint_parser: Callable[
            [str, Dict[str, object]], Optional[FailureFingerprint]
        ],
        environment_provider: Callable[
            [DiagnosisRequest, tuple], Dict[str, Any]
        ],
    ) -> None:
        self.cmake = cmake
        self.store_factory = store_factory
        self.reporter_factory = reporter_factory
        self.probe_factory = probe_factory
        self.fingerprint_parser = fingerprint_parser
        self.environment_provider = environment_provider

    def diagnose(
        self,
        raw_request: DiagnosisRequest,
        previous: Optional[Session] = None,
    ) -> Session:
        request = raw_request.normalized()
        session_id = previous.session_id if previous else new_session_id()
        report_root = Path(request.report_dir)
        store = self.store_factory(report_root)
        reporter = self.reporter_factory(report_root)
        session = previous or Session(
            session_id,
            utc_now(),
            utc_now(),
            SessionStatus.INSPECTING,
            asdict(request),
        )
        session.request = asdict(request)
        session.environment.update(
            self.environment_provider(request, self.cmake.version())
        )
        session.status = SessionStatus.INSPECTING
        session.errors = []
        store.save(session)

        build_root = Path(request.work_dir) / "sessions" / session_id / "build"
        baseline = self._build(request, build_root / "baseline", False)
        session.commands.extend(
            _new_records(session.commands, baseline.configure, baseline.build)
        )
        if not baseline.succeeded:
            session.status = SessionStatus.BASELINE_FAILED
            session.errors.append(
                "普通构建在 {} 阶段失败；未进入 Unity 最小化。".format(
                    baseline.failed_stage
                )
            )
            return _finish(session, store, reporter)

        unity = self._build(request, build_root / "unity", True)
        session.commands.extend(
            _new_records(session.commands, unity.configure, unity.build)
        )
        if unity.succeeded:
            session.status = SessionStatus.NOT_REPRODUCED
            session.cases = []
            return _finish(session, store, reporter)
        if unity.failed_stage != "build":
            session.status = SessionStatus.UNSUPPORTED_STAGE
            session.errors.append("Unity configure 失败，无法执行编译冲突诊断。")
            return _finish(session, store, reporter)

        units = self._relevant_units(unity)
        if not units:
            session.status = SessionStatus.UNSUPPORTED_STAGE
            session.errors.append(
                "未找到可映射的 Unity 编译单元；请确认生成器支持编译数据库。"
            )
            return _finish(session, store, reporter)

        session.status = SessionStatus.MINIMIZING
        old_cases = {item.unity_source: item for item in session.cases}
        cache = {probe.key: probe for item in session.cases for probe in item.probes}
        session.cases = self._diagnose_units(
            request,
            session,
            units,
            unity.log_text(),
            old_cases,
            cache,
            store,
        )
        partial = {"NON_REPLAYABLE", "BUDGET_EXHAUSTED"}
        session.status = (
            SessionStatus.PARTIAL
            if any(item.status in partial for item in session.cases)
            else SessionStatus.COMPLETE
        )
        return _finish(session, store, reporter)

    def _build(self, request: DiagnosisRequest, build_dir: Path, unity: bool) -> Any:
        return self.cmake.configure_and_build(
            Path(request.source),
            build_dir,
            unity,
            request.generator or None,
            request.config,
            request.targets,
            request.cmake_args,
            request.timeout,
        )

    def _relevant_units(self, unity: Any) -> List[Any]:
        units = self.cmake.discover_unity_units(unity.build_dir)
        log = unity.log_text()
        referenced = [
            unit
            for unit in units
            if str(unit.unity_source) in log or unit.unity_source.name in log
        ]
        return referenced or units

    def _diagnose_units(
        self,
        request: DiagnosisRequest,
        session: Session,
        units: List[Any],
        build_log: str,
        old_cases: Dict[str, ConflictCase],
        cache: Dict[str, ProbeRecord],
        store: Any,
    ) -> List[ConflictCase]:
        cases: List[ConflictCase] = []
        cmake_version = self.cmake.version()
        for index, unit in enumerate(units, 1):
            case = old_cases.get(str(unit.unity_source)) or ConflictCase(
                "CASE-{:03d}".format(index),
                unit.target,
                unit.language,
                str(unit.unity_source),
                [str(item) for item in unit.ordered_sources],
            )
            case.ordered_sources = [str(item) for item in unit.ordered_sources]
            cases.append(case)
            session.cases = cases
            store.save(session)
            case.fingerprint = self.fingerprint_parser(
                build_log, unit.compile_entry
            )
            if not case.fingerprint:
                case.status = "NON_REPLAYABLE"
                case.notes.append("构建日志中没有可解析的编译器主错误。")
                continue
            probe_runner = self.probe_factory(
                Path(request.work_dir)
                / "sessions"
                / session.session_id
                / "probes"
                / case.case_id,
                request.timeout,
            )

            def run_probe(sources: Sequence[str]) -> ProbeRecord:
                result = probe_runner.run(
                    sources, unit.compile_entry, case.fingerprint, cache
                )
                cache[result.key] = result
                case.probes = _deduplicate_probes(case.probes + [result])
                store.save(session)
                return result

            result = minimize_ordered(
                case.ordered_sources,
                run_probe,
                request.max_probes,
                request.timeout,
            )
            case.status = result.status
            case.minimal_sources = result.sources
            case.order_sensitive = result.order_sensitive
            case.probes = _deduplicate_probes(case.probes + result.probes)
            texts = {
                item: Path(item).read_text(encoding="utf-8", errors="replace")
                for item in case.minimal_sources
                if Path(item).is_file()
            }
            case.classification = classify_case(case, texts)
            case.suggestions = build_suggestions(
                case, cmake_version, Path(request.source)
            )
            store.save(session)
        return cases


def exit_code_for(session: Session) -> int:
    return {
        SessionStatus.COMPLETE: 0,
        SessionStatus.NOT_REPRODUCED: 0,
        SessionStatus.VERIFIED: 0,
        SessionStatus.BASELINE_FAILED: 2,
        SessionStatus.PARTIAL: 3,
        SessionStatus.UNSUPPORTED_STAGE: 4,
        SessionStatus.VERIFY_FAILED: 5,
    }.get(session.status, 1)


def new_session_id() -> str:
    timestamp = utc_now()[:19].replace(":", "").replace("-", "")
    return "{}-{}".format(timestamp, uuid.uuid4().hex[:8])


def _finish(session: Session, store: Any, reporter: Any) -> Session:
    store.save(session)
    reporter.render(session)
    return session


def _new_records(existing: List[Any], *records: Any) -> List[Any]:
    known = {(item.log_path, tuple(item.argv)) for item in existing}
    return [
        item
        for item in records
        if item is not None and (item.log_path, tuple(item.argv)) not in known
    ]


def _deduplicate_probes(records: List[ProbeRecord]) -> List[ProbeRecord]:
    result: Dict[str, ProbeRecord] = {}
    for record in records:
        result[record.key] = record
    return list(result.values())
