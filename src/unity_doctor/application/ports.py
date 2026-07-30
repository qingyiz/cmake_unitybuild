from __future__ import annotations

from pathlib import Path
from typing import Dict, List, Optional, Protocol, Sequence

from unity_doctor.domain.models import (
    CommandRecord,
    FailureFingerprint,
    ProbeRecord,
    Session,
)


class SessionStore(Protocol):
    def save(self, session: Session) -> Path:
        ...

    def load(self, session_path: Path) -> Session:
        ...


class Reporter(Protocol):
    def render(self, session: Session) -> Dict[str, Path]:
        ...


class ProbeRunner(Protocol):
    def run(
        self,
        sources: Sequence[str],
        compile_entry: Dict[str, object],
        expected: FailureFingerprint,
        cache: Optional[Dict[str, ProbeRecord]] = None,
    ) -> ProbeRecord:
        ...


class CommandExecutor(Protocol):
    def run(
        self,
        argv: List[str],
        cwd: Path,
        log_path: Path,
        timeout_seconds: Optional[float] = None,
    ) -> CommandRecord:
        ...
