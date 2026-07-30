from __future__ import annotations

from pathlib import Path
from typing import List, Tuple

from unity_doctor.domain.models import ConflictCase, Suggestion


def build_suggestions(
    case: ConflictCase,
    cmake_version: Tuple[int, int, int],
    source_root: Path,
) -> List[Suggestion]:
    sources = case.minimal_sources or case.ordered_sources
    relative = [_relative_or_absolute(Path(item), source_root) for item in sources]
    target = case.target
    supports_target_directory = cmake_version >= (3, 18, 0)
    source_lines = "\n".join('    "{}"'.format(_cmake_escape(item)) for item in relative)
    if supports_target_directory:
        skip_code = (
            "set_source_files_properties(\n"
            "{sources}\n"
            '    TARGET_DIRECTORY "{target}"\n'
            "    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON\n"
            ")"
        ).format(sources=source_lines, target=_cmake_escape(target))
        skip_guidance = "放在 target 已创建之后；TARGET_DIRECTORY 会把属性设置到 target 的目录作用域。"
    else:
        skip_code = (
            "set_source_files_properties(\n"
            "{sources}\n"
            "    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON\n"
            ")"
        ).format(sources=source_lines)
        skip_guidance = "CMake <3.18 时必须把片段放到创建该 target 的同一 CMakeLists.txt 目录作用域。"
    suggestions = [
        Suggestion(
            "SOURCE_EXCLUSION",
            "low",
            "仅将最小冲突集合排除出 Unity，保留 target 其他源文件的加速收益。",
            "3.16",
            cmake_version >= (3, 16, 0),
            skip_code if cmake_version >= (3, 16, 0) else "",
            skip_guidance,
        ),
        Suggestion(
            "REDUCE_BATCH",
            "low",
            "先把 Unity batch 降到 2 或 4，减少冲突面和单次编译内存。",
            "3.16",
            cmake_version >= (3, 16, 0),
            'set_property(TARGET "{}" PROPERTY UNITY_BUILD_BATCH_SIZE 4)'.format(
                _cmake_escape(target)
            ),
            "Unity 是否开启仍由开发者通过 CMAKE_UNITY_BUILD 控制。",
        ),
        Suggestion(
            "EXPLICIT_GROUP",
            "medium",
            "只把已验证兼容的源文件放入显式 Unity group。",
            "3.18",
            cmake_version >= (3, 18, 0),
            (
                'set_property(TARGET "{target}" PROPERTY UNITY_BUILD_MODE GROUP)\n'
                "# 为已验证兼容的 source 设置相同 UNITY_GROUP；未分组 source 单独编译"
            ).format(target=_cmake_escape(target))
            if cmake_version >= (3, 18, 0)
            else "",
            "GROUP 模式下未设置 UNITY_GROUP 的源文件会单独编译。",
        ),
        Suggestion(
            "DISABLE_TARGET",
            "medium",
            "若该 target 大部分源文件均不兼容，显式关闭该 target 的 Unity。",
            "3.16",
            cmake_version >= (3, 16, 0),
            'set_property(TARGET "{}" PROPERTY UNITY_BUILD OFF)'.format(
                _cmake_escape(target)
            ),
            "适合止血，但不会获得该 target 的 Unity 加速收益。",
        ),
    ]
    classification = case.classification.category if case.classification else "UNKNOWN"
    guidance = _source_guidance(classification)
    if guidance:
        suggestions.append(
            Suggestion(
                "SOURCE_REFACTOR",
                "high",
                "人工修复代码根因后可让源文件重新进入 Unity。",
                "0.0",
                True,
                "",
                guidance,
            )
        )
    if classification == "ANONYMOUS_NAMESPACE":
        available = cmake_version >= (3, 20, 0)
        suggestions.append(
            Suggestion(
                "UNIQUE_ID",
                "high",
                "为匿名命名空间增加每源唯一子命名空间。",
                "3.20",
                available,
                (
                    'set_property(TARGET "{}" PROPERTY '
                    "UNITY_BUILD_UNIQUE_ID UNITY_BUILD_ID)"
                ).format(_cmake_escape(target))
                if available
                else "",
                "该属性需要源码使用 UNITY_BUILD_ID 包裹冲突名字；工具不会自动改源码。",
            )
        )
    return suggestions


def _source_guidance(category: str) -> str:
    return {
        "TU_LOCAL_NAME": "重命名文件级 static 标识符，或将实现移入具有项目语义的唯一命名空间。",
        "ANONYMOUS_NAMESPACE": "重命名冲突标识符，或配合 UNITY_BUILD_UNIQUE_ID 增加唯一子命名空间。",
        "MACRO_LEAK": "缩小宏作用域，在源文件末尾 #undef，避免用全局宏表达局部常量。",
        "INCLUDE_ORDER": "为使用到的类型/声明增加直接 include，移除对前序源文件的依赖。",
        "HEADER_DEFINITION": "增加 include guard/#pragma once，并把头文件定义改为 inline 或移到单一源文件。",
        "PER_SOURCE_OPTIONS": "恢复逐文件 COMPILE_DEFINITIONS/COMPILE_OPTIONS；CMake 会将其排除出 Unity。",
        "QT_GENERATED_SOURCE": "检查 AUTOMOC/AUTOGEN 配置，避免同时手工 include moc_*.cpp 和自动生成。",
    }.get(category, "")


def _relative_or_absolute(path: Path, root: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return path.resolve().as_posix()


def _cmake_escape(value: str) -> str:
    return value.replace("\\", "/").replace('"', '\\"').replace(";", "\\;")
