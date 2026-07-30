from __future__ import annotations

import subprocess
import time
from pathlib import Path
from typing import List, Optional

from unity_doctor.domain.models import CommandRecord
from unity_doctor.timeutil import utc_now


class SubprocessExecutor:
    """Execute argv directly and persist combined output for audit."""

    def run(
        self,
        argv: List[str],
        cwd: Path,
        log_path: Path,
        timeout_seconds: Optional[float] = None,
    ) -> CommandRecord:
        cwd = cwd.resolve()
        log_path.parent.mkdir(parents=True, exist_ok=True)
        started_at = utc_now()
        started = time.monotonic()
        timed_out = False
        try:
            completed = subprocess.run(
                argv,
                cwd=str(cwd),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                errors="replace",
                timeout=timeout_seconds,
                check=False,
            )
            output = completed.stdout
            exit_code = completed.returncode
        except subprocess.TimeoutExpired as exc:
            timed_out = True
            output = _timeout_output(exc)
            exit_code = 124
        log_path.write_text(output, encoding="utf-8")
        return CommandRecord(
            argv=list(argv),
            cwd=str(cwd),
            exit_code=exit_code,
            started_at=started_at,
            duration_seconds=round(time.monotonic() - started, 6),
            log_path=str(log_path.resolve()),
            timed_out=timed_out,
        )


def _timeout_output(exc: subprocess.TimeoutExpired) -> str:
    output = exc.stdout or ""
    if isinstance(output, bytes):
        output = output.decode(errors="replace")
    return "{}\n[unity-build-doctor] command timed out\n".format(output)
