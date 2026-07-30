from __future__ import annotations

import math
import time
from dataclasses import dataclass, field
from typing import Callable, List, Sequence

from unity_doctor.domain.models import ProbeRecord


@dataclass(frozen=True)
class MinimizationResult:
    status: str
    sources: List[str]
    probes: List[ProbeRecord] = field(default_factory=list)
    order_sensitive: bool = False


def minimize_ordered(
    candidates: Sequence[str],
    probe: Callable[[Sequence[str]], ProbeRecord],
    max_probes: int = 100,
    timeout_seconds: float = 300.0,
) -> MinimizationResult:
    """Return a 1-minimal ordered set for the probe's target fingerprint."""

    started = time.monotonic()
    records: List[ProbeRecord] = []

    def run(items: Sequence[str]) -> ProbeRecord:
        if len(records) >= max_probes or time.monotonic() - started >= timeout_seconds:
            raise _BudgetExhausted
        result = probe(items)
        records.append(result)
        return result

    current = list(candidates)
    if not current:
        return MinimizationResult("NON_REPLAYABLE", [], records)
    try:
        if not run(current).reproduced:
            return MinimizationResult("NON_REPLAYABLE", current, records)
        if len(current) == 1:
            return MinimizationResult("MINIMIZED", current, records)
        granularity = 2
        while len(current) >= 2:
            parts = _contiguous_parts(current, granularity)
            reduced = False
            for part in parts:
                if run(part).reproduced:
                    current = list(part)
                    granularity = max(granularity - 1, 2)
                    reduced = True
                    break
            if reduced:
                continue
            for part in parts:
                part_set = set(part)
                complement = [item for item in current if item not in part_set]
                if complement and run(complement).reproduced:
                    current = complement
                    granularity = max(granularity - 1, 2)
                    reduced = True
                    break
            if reduced:
                continue
            if granularity >= len(current):
                break
            granularity = min(len(current), granularity * 2)
        for index in range(len(current)):
            without = current[:index] + current[index + 1 :]
            if without and run(without).reproduced:
                current = without
                return minimize_ordered(
                    current,
                    probe,
                    max_probes=max(1, max_probes - len(records)),
                    timeout_seconds=max(0.001, timeout_seconds - (time.monotonic() - started)),
                )
        reverse_result = run(list(reversed(current))) if len(current) > 1 else None
        order_sensitive = bool(reverse_result and not reverse_result.reproduced)
        return MinimizationResult("MINIMIZED", current, records, order_sensitive)
    except _BudgetExhausted:
        return MinimizationResult("BUDGET_EXHAUSTED", current, records)


def _contiguous_parts(items: Sequence[str], count: int) -> List[List[str]]:
    size = int(math.ceil(len(items) / float(count)))
    return [list(items[index : index + size]) for index in range(0, len(items), size)]


class _BudgetExhausted(Exception):
    pass
