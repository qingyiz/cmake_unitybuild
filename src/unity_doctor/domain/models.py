from __future__ import annotations

from dataclasses import asdict, dataclass, field
from enum import Enum
from typing import Any, Dict, List, Optional


SCHEMA_VERSION = 1


class SessionStatus(str, Enum):
    INSPECTING = "INSPECTING"
    BASELINE_FAILED = "BASELINE_FAILED"
    NOT_REPRODUCED = "NOT_REPRODUCED"
    DISCOVERING = "DISCOVERING"
    MINIMIZING = "MINIMIZING"
    PARTIAL = "PARTIAL"
    UNSUPPORTED_STAGE = "UNSUPPORTED_STAGE"
    COMPLETE = "COMPLETE"
    VERIFY_FAILED = "VERIFY_FAILED"
    VERIFIED = "VERIFIED"


@dataclass(frozen=True)
class CommandRecord:
    argv: List[str]
    cwd: str
    exit_code: int
    started_at: str
    duration_seconds: float
    log_path: str
    timed_out: bool = False


@dataclass(frozen=True)
class FailureFingerprint:
    compiler_family: str
    phase: str
    category: str
    symbol: str
    message: str

    @property
    def key(self) -> str:
        return "|".join(
            (
                self.compiler_family,
                self.phase,
                self.category,
                self.symbol,
                self.message,
            )
        )


@dataclass(frozen=True)
class ProbeRecord:
    key: str
    sources: List[str]
    command: CommandRecord
    reproduced: bool
    fingerprint: Optional[FailureFingerprint]


@dataclass(frozen=True)
class Classification:
    category: str
    confidence: float
    summary: str
    evidence: List[str] = field(default_factory=list)
    alternatives: List[str] = field(default_factory=list)


@dataclass(frozen=True)
class Suggestion:
    kind: str
    risk: str
    summary: str
    minimum_cmake: str
    available: bool
    cmake: str = ""
    guidance: str = ""


@dataclass
class ConflictCase:
    case_id: str
    target: str
    language: str
    unity_source: str
    ordered_sources: List[str]
    fingerprint: Optional[FailureFingerprint] = None
    status: str = "DISCOVERED"
    minimal_sources: List[str] = field(default_factory=list)
    order_sensitive: bool = False
    probes: List[ProbeRecord] = field(default_factory=list)
    classification: Optional[Classification] = None
    suggestions: List[Suggestion] = field(default_factory=list)
    notes: List[str] = field(default_factory=list)


@dataclass
class Session:
    session_id: str
    created_at: str
    updated_at: str
    status: SessionStatus
    request: Dict[str, Any]
    environment: Dict[str, Any] = field(default_factory=dict)
    commands: List[CommandRecord] = field(default_factory=list)
    cases: List[ConflictCase] = field(default_factory=list)
    errors: List[str] = field(default_factory=list)
    verification: Dict[str, Any] = field(default_factory=dict)
    schema_version: int = SCHEMA_VERSION

    def to_dict(self) -> Dict[str, Any]:
        return _primitive(asdict(self))

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Session":
        cases: List[ConflictCase] = []
        for raw_case in data.get("cases", []):
            fingerprint = _fingerprint(raw_case.get("fingerprint"))
            probes = [
                ProbeRecord(
                    key=item["key"],
                    sources=list(item.get("sources", [])),
                    command=_command(item["command"]),
                    reproduced=bool(item.get("reproduced")),
                    fingerprint=_fingerprint(item.get("fingerprint")),
                )
                for item in raw_case.get("probes", [])
            ]
            raw_classification = raw_case.get("classification")
            classification = (
                Classification(**raw_classification) if raw_classification else None
            )
            suggestions = [
                Suggestion(**item) for item in raw_case.get("suggestions", [])
            ]
            cases.append(
                ConflictCase(
                    case_id=raw_case["case_id"],
                    target=raw_case.get("target", "unknown"),
                    language=raw_case.get("language", "CXX"),
                    unity_source=raw_case.get("unity_source", ""),
                    ordered_sources=list(raw_case.get("ordered_sources", [])),
                    fingerprint=fingerprint,
                    status=raw_case.get("status", "DISCOVERED"),
                    minimal_sources=list(raw_case.get("minimal_sources", [])),
                    order_sensitive=bool(raw_case.get("order_sensitive")),
                    probes=probes,
                    classification=classification,
                    suggestions=suggestions,
                    notes=list(raw_case.get("notes", [])),
                )
            )
        return cls(
            session_id=data["session_id"],
            created_at=data["created_at"],
            updated_at=data["updated_at"],
            status=SessionStatus(data["status"]),
            request=dict(data.get("request", {})),
            environment=dict(data.get("environment", {})),
            commands=[_command(item) for item in data.get("commands", [])],
            cases=cases,
            errors=list(data.get("errors", [])),
            verification=dict(data.get("verification", {})),
            schema_version=int(data.get("schema_version", SCHEMA_VERSION)),
        )


def _command(data: Dict[str, Any]) -> CommandRecord:
    return CommandRecord(
        argv=list(data.get("argv", [])),
        cwd=data.get("cwd", ""),
        exit_code=int(data.get("exit_code", 1)),
        started_at=data.get("started_at", ""),
        duration_seconds=float(data.get("duration_seconds", 0)),
        log_path=data.get("log_path", ""),
        timed_out=bool(data.get("timed_out")),
    )


def _fingerprint(data: Optional[Dict[str, Any]]) -> Optional[FailureFingerprint]:
    return FailureFingerprint(**data) if data else None


def _primitive(value: Any) -> Any:
    if isinstance(value, Enum):
        return value.value
    if isinstance(value, dict):
        return {key: _primitive(item) for key, item in value.items()}
    if isinstance(value, list):
        return [_primitive(item) for item in value]
    return value
