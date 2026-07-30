from __future__ import annotations

import re
from typing import List, Mapping, Optional, Sequence

from unity_doctor.domain.models import Classification, ConflictCase


def classify_case(
    case: ConflictCase,
    source_texts: Mapping[str, str],
    compile_signatures: Optional[Mapping[str, str]] = None,
    hints: Optional[Mapping[str, object]] = None,
) -> Classification:
    sources = case.minimal_sources or case.ordered_sources
    contents = [source_texts.get(item, "") for item in sources]
    hints = hints or {}
    fingerprint = case.fingerprint
    category = fingerprint.category if fingerprint else ""
    symbol = fingerprint.symbol if fingerprint else ""
    paths = " ".join(sources).lower()

    if len(sources) == 1:
        return Classification(
            "SINGLE_SOURCE",
            0.95,
            "单个源文件即可复现，不能归因于两个翻译单元之间的名字冲突。",
            [sources[0]],
        )
    if any(token in paths for token in ("moc_", "_autogen", "qrc_", ".moc")):
        return Classification(
            "QT_GENERATED_SOURCE",
            0.95,
            "冲突集合包含 Qt/AUTOGEN 风格生成源。",
            [item for item in sources if _looks_generated(item)],
        )
    if compile_signatures:
        signatures = {compile_signatures.get(item, "") for item in sources}
        if len(signatures - {""}) > 1:
            return Classification(
                "PER_SOURCE_OPTIONS",
                0.95,
                "冲突源文件的有效编译上下文不同，不应直接放入同一 Unity 单元。",
                ["发现 {} 个不同编译签名".format(len(signatures - {""}))],
            )
    if hints.get("header_without_guard"):
        return Classification(
            "HEADER_DEFINITION",
            0.9,
            "诊断指向缺少保护或包含非 inline 定义的头文件。",
            [str(hints["header_without_guard"])],
        )
    if _macro_interaction(contents, category):
        return Classification(
            "MACRO_LEAK",
            0.9,
            "前序源文件留下的宏状态改变了后续源文件的预处理结果。",
            _matching_lines(sources, contents, r"^\s*#\s*(define|undef|if|ifdef|error)\b"),
        )
    anonymous = [text for text in contents if re.search(r"namespace\s*\{", text)]
    if category == "redefinition" and len(anonymous) >= 2:
        return Classification(
            "ANONYMOUS_NAMESPACE",
            0.95,
            "多个源文件的匿名命名空间在 Unity 翻译单元中合并，产生同名定义。",
            _symbol_evidence(sources, contents, symbol, "namespace {"),
        )
    if category == "redefinition" and _static_symbol_count(contents, symbol) >= 2:
        return Classification(
            "TU_LOCAL_NAME",
            0.95,
            "多个原翻译单元使用了同名文件级 static 定义。",
            _symbol_evidence(sources, contents, symbol, "static"),
        )
    if category in {"missing_declaration", "incomplete_type", "missing_include"}:
        return Classification(
            "INCLUDE_ORDER",
            0.8,
            "诊断表明源文件依赖 include 顺序、间接包含或前置声明状态。",
            [fingerprint.message if fingerprint else category],
            ["也可能是 qmake→CMake 迁移时遗漏 include directory"],
        )
    if category == "redefinition" and hints.get("header_path"):
        return Classification(
            "HEADER_DEFINITION",
            0.75,
            "重定义位置来自头文件，可能缺少 include guard 或包含非 inline 定义。",
            [str(hints["header_path"])],
        )
    return Classification(
        "UNKNOWN",
        0.35,
        "现有诊断不足以安全确定根因。",
        [fingerprint.message] if fingerprint else [],
        [
            "翻译单元局部名字冲突",
            "宏或 include 顺序影响",
            "生成源或逐文件编译选项差异",
        ],
    )


def _macro_interaction(contents: Sequence[str], category: str) -> bool:
    has_define = any(re.search(r"^\s*#\s*define\b", text, re.MULTILINE) for text in contents)
    has_consumer = any(
        re.search(r"^\s*#\s*(if|ifdef|ifndef|error)\b", text, re.MULTILINE)
        for text in contents
    )
    return has_define and has_consumer and category in {
        "preprocessor",
        "macro",
        "redefinition",
        "compiler_error",
    }


def _static_symbol_count(contents: Sequence[str], symbol: str) -> int:
    pattern = (
        r"\bstatic\b[^\n;{{}}]*\b{}\b".format(re.escape(symbol))
        if symbol
        else r"\bstatic\b"
    )
    return sum(bool(re.search(pattern, text)) for text in contents)


def _matching_lines(
    sources: Sequence[str], contents: Sequence[str], pattern: str
) -> List[str]:
    result: List[str] = []
    compiled = re.compile(pattern)
    for source, text in zip(sources, contents):
        for number, line in enumerate(text.splitlines(), 1):
            if compiled.search(line):
                result.append("{}:{}: {}".format(source, number, line.strip()))
                break
    return result[:6]


def _symbol_evidence(
    sources: Sequence[str], contents: Sequence[str], symbol: str, fallback: str
) -> List[str]:
    return _matching_lines(
        sources, contents, re.escape(symbol) if symbol else re.escape(fallback)
    )


def _looks_generated(path: str) -> bool:
    lowered = path.lower()
    return any(token in lowered for token in ("moc_", "_autogen", "qrc_", ".moc"))
