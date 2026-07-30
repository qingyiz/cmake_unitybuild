from __future__ import annotations

from pathlib import Path
from typing import Dict, Optional

from unity_doctor.adapters.cmake import CMakeAdapter
from unity_doctor.adapters.compiler import (
    CompilerProbeRunner,
    detect_compiler_family,
    parse_failure_fingerprint,
)
from unity_doctor.adapters.environment import collect_environment
from unity_doctor.adapters.process import SubprocessExecutor
from unity_doctor.adapters.reporting import ArtifactReporter, JsonSessionStore
from unity_doctor.application.diagnosis import DiagnosisService
from unity_doctor.application.requests import DiagnosisRequest, VerificationRequest
from unity_doctor.application.verification import VerificationService
from unity_doctor.domain.models import FailureFingerprint, Session


def diagnose(request: DiagnosisRequest, previous: Optional[Session] = None) -> Session:
    return _diagnosis_service().diagnose(request, previous)


def resume(
    session_path: Path,
    max_probes: Optional[int] = None,
    timeout: Optional[float] = None,
) -> Session:
    session_path = session_path.expanduser().resolve()
    report_root = session_path.parent.parent
    previous = JsonSessionStore(report_root).load(session_path)
    saved = DiagnosisRequest(**previous.request)
    request = DiagnosisRequest(
        **{
            **saved.__dict__,
            "max_probes": max_probes
            if max_probes is not None
            else saved.max_probes + 100,
            "timeout": timeout if timeout is not None else saved.timeout,
        }
    )
    return diagnose(request, previous)


def verify(request: VerificationRequest) -> Session:
    return VerificationService(
        CMakeAdapter(),
        SubprocessExecutor(),
        JsonSessionStore,
        ArtifactReporter,
    ).verify(request)


def _diagnosis_service() -> DiagnosisService:
    return DiagnosisService(
        CMakeAdapter(),
        JsonSessionStore,
        ArtifactReporter,
        lambda root, timeout: CompilerProbeRunner(root, timeout),
        _fingerprint,
        collect_environment,
    )


def _fingerprint(
    output: str, compile_entry: Dict[str, object]
) -> Optional[FailureFingerprint]:
    return parse_failure_fingerprint(
        output, detect_compiler_family(compile_entry)
    )
