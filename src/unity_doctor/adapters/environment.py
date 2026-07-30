from __future__ import annotations

import hashlib
import platform
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, Tuple

from unity_doctor.application.requests import DiagnosisRequest


def collect_environment(
    request: DiagnosisRequest, cmake_version: Tuple[int, int, int]
) -> Dict[str, Any]:
    return {
        "cmake_version": ".".join(str(item) for item in cmake_version),
        "python_version": platform.python_version(),
        "platform": platform.platform(),
        "machine": platform.machine(),
        "source_revision": _source_revision(Path(request.source)),
        "argv_runtime": sys.executable,
    }


def _source_revision(source: Path) -> Dict[str, Any]:
    try:
        head = subprocess.run(
            ["git", "-C", str(source), "rev-parse", "HEAD"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=5,
            check=False,
        ).stdout.strip()
        dirty = subprocess.run(
            ["git", "-C", str(source), "status", "--porcelain"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=5,
            check=False,
        ).stdout
        if head:
            return {"git_head": head, "dirty": bool(dirty.strip())}
    except (OSError, subprocess.SubprocessError):
        pass
    digest = hashlib.sha256()
    count = 0
    for path in sorted(source.rglob("*")):
        relative = path.relative_to(source)
        if path.is_file() and not any(part.startswith(".") for part in relative.parts):
            digest.update(str(relative).encode())
            digest.update(path.read_bytes())
            count += 1
    return {"tree_sha256": digest.hexdigest(), "file_count": count}
