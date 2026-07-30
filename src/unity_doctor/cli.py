from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import List, Optional

from unity_doctor import __version__
from unity_doctor.application.diagnosis import (
    exit_code_for,
)
from unity_doctor.application.requests import DiagnosisRequest, VerificationRequest
from unity_doctor.composition import diagnose, resume, verify


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="unity-build-doctor",
        description="只读诊断 CMake Unity Build 冲突",
    )
    parser.add_argument("--version", action="version", version=__version__)
    subparsers = parser.add_subparsers(dest="command")

    diagnose = subparsers.add_parser("diagnose", help="运行普通/Unity 双构建并诊断")
    diagnose.add_argument("--source", required=True, help="CMake 源码目录")
    diagnose.add_argument("--work-dir", required=True, help="独立诊断构建根目录")
    diagnose.add_argument("--report-dir", help="报告根目录，默认 <work-dir>/reports")
    diagnose.add_argument("--generator", help="CMake generator，例如 Ninja")
    diagnose.add_argument("--config", default="Debug", help="构建配置")
    diagnose.add_argument("--target", action="append", default=[], help="目标，可重复")
    diagnose.add_argument(
        "--cmake-arg", action="append", default=[], help="额外 CMake configure 参数"
    )
    diagnose.add_argument("--max-probes", type=int, default=100)
    diagnose.add_argument("--timeout", type=float, default=300.0)

    resume = subparsers.add_parser("resume", help="恢复未完成的诊断会话")
    resume.add_argument("--session", required=True, help="session.json 路径")
    resume.add_argument("--max-probes", type=int, help="本次恢复的探针预算")
    resume.add_argument("--timeout", type=float, help="本次恢复的命令/最小化超时")

    verify = subparsers.add_parser("verify", help="验证人工应用的缓解方案")
    verify.add_argument("--source", required=True)
    verify.add_argument("--work-dir", required=True)
    verify.add_argument("--generator")
    verify.add_argument("--config", default="Debug")
    verify.add_argument("--target", action="append", default=[])
    verify.add_argument("--cmake-arg", action="append", default=[])
    verify.add_argument("--test-command", help="测试命令，不经 shell 执行")
    verify.add_argument("--benchmark-runs", type=int, default=0)
    verify.add_argument("--timeout", type=float, default=300.0)
    return parser


def main(argv: Optional[List[str]] = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.command is None:
        parser.print_help()
        return 0
    try:
        if args.command == "diagnose":
            request = DiagnosisRequest(
                source=args.source,
                work_dir=args.work_dir,
                report_dir=args.report_dir or "",
                generator=args.generator or "",
                config=args.config,
                targets=args.target,
                cmake_args=args.cmake_arg,
                max_probes=args.max_probes,
                timeout=args.timeout,
            )
            session = diagnose(request)
        elif args.command == "resume":
            session = resume(
                Path(args.session), args.max_probes, args.timeout
            )
        else:
            request = VerificationRequest(
                source=args.source,
                work_dir=args.work_dir,
                generator=args.generator or "",
                config=args.config,
                targets=args.target,
                cmake_args=args.cmake_arg,
                test_command=args.test_command or "",
                benchmark_runs=args.benchmark_runs,
                timeout=args.timeout,
            )
            session = verify(request)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print("unity-build-doctor: {}".format(exc), file=sys.stderr)
        return 2
    report_dir = Path(session.request["report_dir"]) / session.session_id
    print("状态：{}".format(session.status.value))
    print("报告：{}".format(report_dir / "report.md"))
    return exit_code_for(session)
