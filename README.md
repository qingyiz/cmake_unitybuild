# Unity Build Doctor

Unity Build Doctor 是一个兼容 Qt 5.15 / Qt 6.4+ 的 C++17 桌面工具，用于排查
C/C++ 项目的 Unity Build 风险。它既可以逐个 target 比较普通构建与 Unity Build，
也可以完全不配置、不构建项目，直接扫描源码中的常见问题。

## 直接使用

macOS 上双击：

`dist/UnityBuildDoctor.app`

然后在界面中完成以下操作：

1. 点击“浏览…”选择项目目录，也可以把目录拖入窗口。
2. 保留自动生成的工作目录。该目录必须位于源码树之外。
3. 在“分析模式”中选择“构建验证（CMake）”或“源码快速扫描（无需构建）”。
4. 使用构建验证时，检查 CMake、生成器和配置是否与目标项目一致。Qt 项目通常需要
   在“附加参数”中加入一行
   `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/compiler_kit`。
5. 点击“开始分析整个项目”。
6. 在结果表中选择 target 或检查项，查看规则、涉及文件、证据、置信度和 CMake 建议；
   可以拖动详情内的分隔条调整正文/建议高度，或点击“专注详情”使用完整主内容区域。
7. 可复制单条 CMake 建议，或导出 JSON、Markdown、CMake 三种报告。

### 源码快速扫描

该模式不要求项目含有 `CMakeLists.txt`，也不会启动 CMake、编译器或其他构建进程。
首批规则包括：

- `UBD-MACRO-001`：源文件结束时仍有效的 `#define`；
- `UBD-MACRO-002`：不同源文件中替换内容不同的活动同名宏；
- `UBD-USING-001`：文件/全局作用域的 `using namespace`；
- `UBD-STATIC-001`：不同源文件中的同名文件作用域 `static` 函数或变量。

扫描会递归读取 `.c/.cc/.cpp/.cxx/.m/.mm`，跳过符号链接、版本控制目录、
常见构建输出目录和超过 2 MiB 的单个文件。结果是静态风险候选，不表示已经确认发生
Unity 编译失败；需要确认真实 target 分组或编译行为时，请使用“构建验证”模式。

仓库内提供了一个可直接测试的工程：

`examples/unity_risk_demo`

它同时包含三类源码风险，普通构建可以通过，Unity Build 会稳定复现文件级
`static` 重定义。详细步骤见
[`examples/unity_risk_demo/README.md`](examples/unity_risk_demo/README.md)。

工作目录内会创建：

- `baseline/`：`CMAKE_UNITY_BUILD=OFF` 的独立构建树；
- `unity/`：`CMAKE_UNITY_BUILD=ON` 的独立构建树；
- `probes/`：最小化冲突集合时生成的临时 Unity driver；
- `analysis.log`：完整的 CMake、编译和探针输出；
- `session.json`：原子保存的当前分析结果。

源码快速扫描只会创建 `analysis.log` 和 `session.json`，不会创建
`baseline/`、`unity/` 或 `probes/`。

工具不会修改目标项目中的源码或 `CMakeLists.txt`。生成的 CMake 片段必须由开发者
审查后手工应用。

## 结果含义

- `Passed`：普通构建和 Unity Build 均通过。
- `Baseline Failed`：关闭 Unity 时已经失败，通常属于迁移、依赖或基础构建问题。
- `Unity Failed`：普通构建通过而 Unity 构建失败，并已尝试重放及最小化。
- `Non Replayable`：真实 Unity 构建失败，但独立探针不能稳定复现相同指纹。
- `Cancelled`：用户取消了分析。
- `Risk Found`：源码快速扫描发现了符合规则的静态风险候选。

当前分类覆盖文件级 `static`、匿名命名空间、宏泄漏、include 顺序、头文件定义、
逐文件编译选项、Qt/生成源、单文件问题和未知问题。

## 本地构建

项目使用 `CMakePresets.json` 统一 Qt 5、Qt 6 的构建配置。仓库中的 Preset
只引用环境变量，不包含开发机绝对路径：

### Qt 5

```bash
export UNITY_DOCTOR_QT5_ROOT=/path/to/Qt/5.15/compiler_kit
cmake --preset qt5-debug
cmake --build --preset qt5-debug
ctest --preset qt5-debug
```

应用位于：

`out/build/qt5-debug/bin/UnityBuildDoctor.app`

### Qt 6

```bash
export UNITY_DOCTOR_QT6_ROOT=/path/to/Qt/6.4/compiler_kit
cmake --preset qt6-debug
cmake --build --preset qt6-debug
ctest --preset qt6-debug
```

应用位于：

`out/build/qt6-debug/bin/UnityBuildDoctor.app`

Release 构建可分别使用 `qt5-release` 和 `qt6-release`。

本机还可以创建不会提交到仓库的 `CMakeUserPresets.json`，继承上述 Preset 并
填写本机 `CMAKE_PREFIX_PATH`。当前工作区已经提供：

- `local-qt5-debug`：Qt 5.15.2；
- `local-qt6-debug`：Qt 6.4.3。

在 VS Code 中执行 `CMake: Select Configure Preset`，选择相应的
`local-qt5-debug` 或 `local-qt6-debug`，再执行配置、构建和测试即可。
`.vscode/settings.json` 已强制 CMake Tools 使用 Preset，避免旧缓存继续绑定
错误的 Qt 路径。

如需手工配置，也可以使用 `UNITY_DOCTOR_QT_MAJOR=5` 或 `6` 明确指定 Qt 主版本；
若指定版本与工具链不匹配，配置阶段会直接失败。

## macOS 部署

生成自包含 macOS 应用：

```bash
./scripts/deploy_macos.sh /path/to/Qt/6.x/macos
```

输出位于：

`dist/UnityBuildDoctor.app`

## 当前验证范围

- 已验证：同一份源码分别使用 macOS arm64 Qt 5.15.2、Qt 6.4.3 构建并通过完整测试；
  CMake 3.27.1、Ninja、Apple Clang 17。
- 开发与部署 `.app` 均包含原生多尺寸 `UnityBuildDoctor.icns` 应用图标。
- 部署包包含 Qt Frameworks 与 `cocoa` platform plugin，不依赖 Qt 安装绝对路径。
- 当前应用使用本地 ad-hoc 签名；未进行 Developer ID 签名、公证或 DMG 打包。
- Windows、Linux、MSVC、GCC 和大型 Qt/AUTOGEN 工程仍需补充原生验证。
- 当前版本不负责把 qmake 工程迁移到 CMake，也不自动应用修复。
