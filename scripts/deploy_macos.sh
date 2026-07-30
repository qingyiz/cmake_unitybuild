#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "用法: $0 /path/to/Qt/6.x/macos" >&2
    exit 2
fi

qt_root="$1"
project_root="$(cd "$(dirname "$0")/.." && pwd)"
build_root="${project_root}/build-release"
dist_root="${project_root}/dist"

cmake -S "${project_root}" -B "${build_root}" -G Ninja \
    -DCMAKE_PREFIX_PATH="${qt_root}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DUNITY_DOCTOR_BUILD_TESTS=OFF
cmake --build "${build_root}" --parallel
cmake --install "${build_root}" --prefix "${dist_root}"
"${qt_root}/bin/macdeployqt" \
    "${dist_root}/UnityBuildDoctor.app" \
    -always-overwrite
codesign --force --deep --sign - "${dist_root}/UnityBuildDoctor.app"
codesign --verify --deep --strict "${dist_root}/UnityBuildDoctor.app"

echo "已生成：${dist_root}/UnityBuildDoctor.app"
