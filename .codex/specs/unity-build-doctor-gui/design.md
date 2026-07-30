# 设计文档：Unity Build Doctor GUI

> 阶段：design
>
> 工作流：design-first
>
> 设计深度：high
>
> 状态：已起草
>
> 最近更新：2026-07-30

## 设计摘要

- 目标：交付原生 Qt/C++ 桌面应用，让用户选择任意 CMake C/C++ 项目后，在界面中完成全项目 target 发现、普通/Unity 双构建、冲突最小化、问题分类与修复建议查看。
- 覆盖行为：REQ-001–REQ-009。
- 核心方案：Qt 5.15/Qt 6.4 Widgets 共源码原生 `.app`，C++17 分层核心，QProcess 驱动目标项目工具链，CMake File API/compile database 获取语义信息，后台 worker 顺序分析全部 buildable target。
- 交付物：`build/bin/UnityBuildDoctor.app` 开发应用束；`dist/UnityBuildDoctor.app` 自包含部署应用束。
- 既有 Python CLI 保留为历史原型和测试思路参考，不是 GUI 的运行时依赖，也不由 GUI 调用。

## 代码库调查

| 证据类型 | 证据 | 已验证事实 | 对设计的影响 |
|---|---|---|---|
| 当前仓库 | `inspect_structure.py` | 已有 Python CLI、21 项测试、8 类 CMake 夹具；没有 C++/Qt 生产代码 | 新增独立 C++ targets；不让 GUI 通过 QProcess 调 Python |
| 用户变更 | “需要有一个界面，可以使用 Qt + C++ 实现……给我一个项目，分析整个项目” | GUI、Qt/C++、全项目分析均为 Must | 废弃 CLI-first 决策，重新定义交互、后台任务和 `.app` |
| Qt 6 Kit | `/Users/qingyizhu/Qt6.4.3/6.4.3/macos/bin/qtpaths --qt-version` | Qt 6.4.3 macOS Kit 可用 | required Qt 6 构建 |
| Qt 5 Kit | `/Users/qingyizhu/Qt5.15.2/bin/qmake -query QT_VERSION` | Qt 5.15.2 macOS arm64 Kit 可用 | required Qt 5 构建 |
| Qt 模块 | `Qt6WidgetsConfig.cmake`、`Qt6TestConfig.cmake` | Widgets 与 Qt Test 可用 | 可实现 Qt Widgets UI 与原生 UI 测试 |
| 部署工具 | `macdeployqt`、`file` | macdeployqt 为 arm64/x86_64 universal，cocoa/offscreen plugins 存在 | 可部署自包含 `.app` 并做 offscreen 测试 |
| 构建工具 | `cmake --version`、`ninja --version` | CMake 3.27.1、Ninja 1.11.1 | 自身使用 CMake/Ninja；目标项目运行时探测自己的 generator/kit |
| 主机 | Apple Clang 17 target triple | macOS arm64 | required 原生交付先锁定 macOS arm64 |
| 应用图标 | 开发包与部署包 `CFBundleIconFile=""`，Resources 无 `.icns` | 当前 bundle 没有应用图标 | `UnityBuildDoctor` target 必须拥有并复制原生图标资源 |

### 工具链与兼容性基线

| 项目 | 已验证值 | 证据 | 设计结论 |
|---|---|---|---|
| GUI Qt | Qt 5.15.2 / Qt 6.4.3 macOS | qtpaths/qmake/Config.cmake | `find_package(QT NAMES Qt6 Qt5)` + requested-major gate |
| C++ | Apple Clang 17，C++17 | `clang++ --version` | 核心使用 C++17，不引入额外第三方库 |
| 目标项目 | CMake C/C++，Qt 版本未知 | 用户场景 | 通过外部 CMake/编译器分析，不链接其库 |
| 开发平台 | macOS arm64 | 本机探测 | `.app` 为 required；Windows/Linux 为后续平台 |

## 约束与设计原则

- 用户操作始终从 GUI 完成；应用启动后不要求终端。
- “整个项目”表示：configure 成功后枚举 codemodel 中全部可构建 C/C++ library/executable target，逐 target 分析；一个 target 失败不能阻断其他 target。
- UI 线程不得运行 CMake、编译器、目录扫描或最小化算法。
- 源码树只读；build/session/report 位于源码树外的缓存或用户指定目录。
- 不自动修改 `.cpp/.h/CMakeLists.txt`；建议支持复制和导出。
- 应用自身 Qt 版本与被分析项目 Qt 版本解耦。
- 无法重放、configure 失败、普通构建失败和取消都作为一等状态显示，不伪造根因。

## 方案比较

| 方案 | 需求覆盖 | 优点 | 代价与风险 | 结论 |
|---|---|---|---|---|
| A. Qt GUI 调用现有 Python CLI | 快速覆盖大部分诊断 | 复用现有实现 | 运行时依赖 Python、进度/取消粒度差，不符合原生 Qt/C++ 预期 | 否决 |
| B. Qt Widgets + C++ 原生诊断核心 | 覆盖全部 Must | 单一 `.app`、原生交互、可控线程/取消、无 Python 依赖 | 需要移植算法与报告模型 | 采用 |
| C. Qt Quick/QML | 可实现现代 UI | 动画和声明式布局 | 增加 QML 部署、测试和团队技术复杂度 | 当前否决，Widgets 足够 |
| D. 静态扫描所有源码但不构建 | 速度快 | 无需配置工具链 | 宏/生成源/编译上下文误报高，无法证明 Unity 专属问题 | 仅作补充证据 |

### DEC-001：采用 Qt 5/6 Widgets/C++17 共源码原生应用

- 上下文与需求：REQ-001–REQ-009。
- 决策：`UnityBuildDoctor` 使用同一套 C++17/Widgets 源码支持 Qt 5.15.2 与 Qt 6.4.3；CMake 通过 `UNITY_DOCTOR_QT_MAJOR=5|6` 选择并校验主版本；不调用 Python。
- 理由：两个本机 Kit 的 Widgets/Test/CMake/macdeployqt 均已验证，用户明确要求双版本。
- 代价：公共源码只能使用 Qt 5.15 与 Qt 6.4 的交集 API；每次构建与测试必须形成双 preset 矩阵。
- 被否决方案：Python CLI wrapper、QML。

### DEC-002：按 target 全项目分析

- 上下文与需求：REQ-002、REQ-004。
- 决策：File API 枚举全部 C/C++ executable/static/shared/module/object target；按 target 运行 baseline 和 Unity 构建并累计结果。
- 理由：一次全局 build 会在首个错误停止，不能代表“整个项目”。
- 代价：大型项目耗时更长；需进度、取消和 target 过滤。
- 被否决方案：只分析用户手填 target、只执行默认 all target。

### DEC-003：后台 worker + 事件流

- 上下文与需求：REQ-003、REQ-008。
- 决策：`AnalysisWorker` 运行于专用 `QThread`；通过 value-type event 向 `AnalysisController` 汇报阶段、target、进度、日志和案件；取消令牌终止当前 QProcess 并停止后续 target。
- 理由：保证 UI 响应和状态可测试。
- 代价：必须明确对象线程归属和退出顺序。
- 被否决方案：主线程同步执行、全局事件循环嵌套。

### DEC-004：双 build tree 与 session 缓存

- 上下文与需求：REQ-003、REQ-007。
- 决策：每个项目 fingerprint 对应缓存根，内部有 `baseline/`、`unity/`、`probes/` 和 `session.json`；不复用用户 build tree。
- 理由：避免污染与 Unity 开关缓存串扰。
- 代价：占用额外磁盘。
- 被否决方案：修改项目现有 build tree。

### DEC-005：Master-detail 分析工作台

- 上下文与需求：REQ-001、REQ-005、REQ-006、REQ-008。
- 决策：窗口由项目设置页和分析工作台组成；工作台左侧 target/问题筛选，中间问题表，右侧详情；底部可折叠实时日志；顶部显示全局进度、开始/取消/重新分析。
- 视觉决策：设置页使用受控最大宽度、页头状态标识、项目/工具链与分析策略卡片、底部行动区；工作台复用相同卡片、主次按钮和状态徽章。颜色、边框、悬停和选择态均由运行时 `QPalette` 派生，禁止为正文硬编码仅适用于浅色模式的颜色。
- 理由：项目级总览、问题定位和证据阅读可以在一屏完成。
- 代价：需要清晰的 model/view 状态同步。
- 被否决方案：多层 wizard、每个问题独立弹窗。

### DEC-006：建议只读、复制与导出

- 上下文与需求：REQ-006、REQ-007。
- 决策：应用生成 CMake 片段和源码修复说明，支持复制与导出 Markdown/JSON/CMake，不提供自动 apply。
- 理由：未知项目结构和未提交改动使自动写入风险过高。
- 代价：用户需要手工应用后重新分析。
- 被否决方案：直接编辑项目。

### DEC-007：使用 bundle 原生 `.icns` 图标

- 上下文与需求：REQ-009（AC-009.7）。
- 决策：保留一份 1024×1024 图标源 PNG，并生成包含 16、32、64、128、256、512、1024 px 表示的 `UnityBuildDoctor.icns`；由 `UnityBuildDoctor` target 以 `MACOSX_PACKAGE_LOCATION=Resources` 复制，并通过 `MACOSX_BUNDLE_ICON_FILE` 写入 `Info.plist`。
- 理由：Finder、Dock 和应用切换器从 bundle 元数据与 Resources 读取图标，运行时只调用 `QApplication::setWindowIcon` 无法修复未启动状态下的默认图标。
- 代价：仓库增加一份 PNG 源资产和一份派生 `.icns`；修改图标时需要重新生成 `.icns`。
- 被否决方案：仅设置运行时窗口图标；它不能覆盖 Finder 中的 bundle 图标。

## 总体架构

```mermaid
flowchart LR
    UI["doctor_ui<br/>Qt Widgets"] --> CTRL["AnalysisController"]
    CTRL --> APP["doctor_application<br/>ProjectAnalysisService"]
    APP --> DOMAIN["doctor_domain<br/>models/ddmin/classifier"]
    APP --> PORTS["ports"]
    INFRA["doctor_infrastructure<br/>CMake/QProcess/File API/session"] --> PORTS
    WORKER["AnalysisWorker / QThread"] --> APP
    APP --> EVENTS["AnalysisEvent stream"]
    EVENTS --> CTRL
```

## 模块与依赖边界

### ARCH-001：doctor_domain

- 单一职责：诊断模型、失败指纹、保序最小化、分类与建议策略。
- 公开契约：标准 C++ value types 与纯函数。
- 允许依赖：C++17 标准库。
- 禁止依赖：Qt Widgets、QProcess、文件系统 UI 状态。
- 目录/target：`cpp/domain/` / `doctor_domain`；测试 `doctor_domain_tests`。

### ARCH-002：doctor_application

- 单一职责：项目分析状态机、逐 target 编排、预算/取消和事件发布。
- 公开契约：`ProjectAnalysisRequest`、`IProjectInspector`、`IBuildRunner`、`IProbeRunner`、`ISessionStore`、`AnalysisEventSink`。
- 允许依赖：doctor_domain、Qt Core（线程安全 value/回调桥）。
- 禁止依赖：Qt Widgets、具体 QProcess/CMake JSON 实现。
- 目录/target：`cpp/application/` / `doctor_application`。

### ARCH-003：doctor_infrastructure

- 单一职责：QProcess 执行、CMake configure/build、File API/compile database、Unity driver、session/export。
- 公开契约：实现 application ports。
- 允许依赖：doctor_application、doctor_domain、Qt Core。
- 禁止依赖：Widgets 和窗口对象。
- 目录/target：`cpp/infrastructure/` / `doctor_infrastructure`。

### ARCH-004：doctor_ui

- 单一职责：用户输入、进度、target/问题 model-view、问题详情、日志与导出交互。
- 公开契约：`MainWindow`、`ProjectSetupWidget`、`AnalysisWorkspaceWidget`、`AnalysisController`。
- 允许依赖：doctor_application/domain、Qt Widgets。
- 禁止依赖：直接创建 QProcess、解析编译器日志或写项目文件。
- 目录/target：`cpp/ui/`，由 `UnityBuildDoctor` executable 所有。

### ARCH-005：composition

- 单一职责：`main.cpp` 创建 adapters/service/controller/window 并连接生命周期。
- 禁止职责：诊断规则、页面布局细节、CMake 命令拼接。
- 边界验证：依赖测试与 review。

### ARCH-006：线程边界

- `MainWindow/Controller` 仅在 GUI thread。
- `AnalysisWorker` 及 service/adapters 在 worker thread；跨线程仅传递注册的 value types/queued signals。
- 取消通过原子 token + worker slot；销毁前先停止 QProcess、退出并 wait QThread。
- 验证：QtTest 线程归属、取消和窗口关闭测试。

### ARCH-007：数据所有权

- `ProjectAnalysisSession` 是规范状态；UI models 只投影 session，不拥有诊断事实。
- 日志、报告和 CMake 建议从同一 session 生成。
- 项目源码只读，session 存放在缓存/用户目录。

## 构建与交付结构

### BUILD-001：顶层 CMake 编排

- `CMakeLists.txt` 只负责 project、C++ 标准、Qt 查找、测试选项、输出根与 `add_subdirectory(cpp)`。
- Qt 路径由 `CMAKE_PREFIX_PATH` 或 Qt Creator Kit 提供，不硬编码用户绝对路径。
- `CMakePresets.json` 定义 Qt 5/Qt 6 configure/build/test presets，build tree 使用 `${sourceDir}/out/build/${presetName}`；本机路径通过 `UNITY_DOCTOR_QT5_ROOT`/`UNITY_DOCTOR_QT6_ROOT` 环境变量或被忽略的 `CMakeUserPresets.json` 提供。
- `UNITY_DOCTOR_QT_MAJOR` 仅允许 `5`/`6`，查找到的 `QT_VERSION_MAJOR` 必须与请求一致。

### BUILD-002：模块 targets

| Target | 类型 | 所有模块 | Public 依赖 | Private 依赖 | 定义位置 |
|---|---|---|---|---|---|
| `doctor_domain` | STATIC | domain | 无 | 无 | `cpp/domain/CMakeLists.txt` |
| `doctor_application` | STATIC | application/ports | doctor_domain、`Qt${QT_VERSION_MAJOR}::Core` | 无 | `cpp/application/CMakeLists.txt` |
| `doctor_infrastructure` | STATIC | adapters | doctor_application/domain、`Qt${QT_VERSION_MAJOR}::Core` | 无 | `cpp/infrastructure/CMakeLists.txt` |
| `UnityBuildDoctor` | MACOSX_BUNDLE | UI/composition | doctor_application/domain | doctor_infrastructure、`Qt${QT_VERSION_MAJOR}::Widgets` | `cpp/app/CMakeLists.txt` |
| `doctor_*_tests` | test executables | 对应模块测试 | 对应 target、`Qt${QT_VERSION_MAJOR}::Test` | 无 | `cpp/tests/CMakeLists.txt` |

### BUILD-003：输出契约

- development：`build/bin/UnityBuildDoctor.app`。
- preset development：`out/build/qt5-debug/bin/UnityBuildDoctor.app` 与 `out/build/qt6-debug/bin/UnityBuildDoctor.app`。
- bundle resource：各开发与部署 `.app` 的 `Contents/Resources/UnityBuildDoctor.icns`，并由 `CFBundleIconFile` 引用。
- test：CTest targets 位于 build tree，不进入应用束。
- install：`stage/UnityBuildDoctor.app`。
- deployed：`dist/UnityBuildDoctor.app`，由当前 preset 对应 Qt 的 `macdeployqt` 收集 frameworks/plugins。

### BUILD-004：目标项目隔离

- 分析目标项目的 build roots 位于用户选择路径或 `QStandardPaths::CacheLocation/projects/<hash>/`。
- 自身 build target 不继承目标项目 include/link/options。
- 所有目标项目命令使用 argv，不经 shell。

### BUILD-005：平台

- required：macOS arm64 `.app`，在本机原生验证。
- optional：Windows `.exe`、Linux ELF/包；本阶段不声称支持。

## 平台与交付矩阵

| 平台/架构 | 开发构建物 | 安装/部署产物 | 发布包 | 运行时依赖 | 原生验证 |
|---|---|---|---|---|---|
| macOS arm64 | `build/bin/UnityBuildDoctor.app` | `dist/UnityBuildDoctor.app` | 本阶段不要求 dmg/pkg | QtCore/Gui/Widgets frameworks、cocoa plugin | bundle 结构、`otool -L`、offscreen smoke、Finder/open 启动 |
| Windows/Linux | 不在 required 范围 | 未定义 | 未定义 | 未验证 | optional 后续 |

### macOS 应用束约束

- `Contents/MacOS/UnityBuildDoctor`。
- `Contents/Info.plist` 包含标识 `com.unitybuilddoctor.app`、显示名、版本与非空 `CFBundleIconFile`。
- `Contents/Resources/UnityBuildDoctor.icns` 包含 macOS 从 16 px 到 1024 px 所需的图标表示。
- `Contents/Frameworks/` 与 `Contents/PlugIns/platforms/libqcocoa.dylib` 由部署阶段验证。
- 当前不要求签名、公证或 DMG。

## 主要界面

### 项目设置页

- 页头：产品标识、标题、用途说明和“只读分析”状态标识。
- 项目目录（选择文件夹/拖放）。
- 诊断工作目录（默认缓存目录，可改）。
- CMake executable、generator、configuration。
- 可选 target 过滤、并行数、最大探针、超时、附加 `-D` 参数。
- 两张视觉卡片承载项目/工具链与分析策略，内容区域设置最大宽度避免超长输入行。
- 底部行动区突出唯一主按钮“开始分析整个项目”，校验错误就地显示。

### 分析工作台

- 顶部：项目名、状态、总进度、当前 target、耗时、开始/取消/重新分析。
- 左侧：总览、全部 target、仅失败 target、问题分类和严重度过滤。
- 中间：target/问题表，列出状态、阶段、问题数、普通/Unity 结果。
- 右侧：问题摘要、最小冲突文件、失败指纹、证据、建议、CMake 片段和复制按钮。
- 底部：实时日志，可按 configure/build/probe 过滤，支持打开日志目录。
- 空状态、首次使用说明、取消/失败恢复均在主窗口内呈现。

## 接口契约

| 接口 | 输入 | 输出/事件 | 错误语义 |
|---|---|---|---|
| `ProjectInspector::inspect` | source/work/CMake args | `ProjectInventory` | configure/file-api/unsupported |
| `ProjectAnalysisService::run` | request + event sink + cancel token | session + progress events | partial/cancelled/complete |
| `BuildRunner::buildTarget` | target/mode/config | command record + diagnostic | configure/compile/link/timeout |
| `ProbeRunner::compile` | ordered sources/context/fingerprint | probe record | reproduced/different/non-replayable |
| `SessionStore::save/load` | session | atomic JSON | corrupt/version mismatch |
| `ReportExporter::exportAll` | session/path | JSON/Markdown/CMake | I/O error |

## 状态与关键流程

```mermaid
stateDiagram-v2
    [*] --> ProjectSetup
    ProjectSetup --> Inspecting: Analyze
    Inspecting --> AnalyzingTargets: configure + codemodel ok
    Inspecting --> ProjectError: configure/file-api fail
    AnalyzingTargets --> AnalyzingTargets: next target
    AnalyzingTargets --> Cancelling: Cancel
    Cancelling --> Cancelled
    AnalyzingTargets --> Complete: all targets visited
    Cancelled --> AnalyzingTargets: Resume
    ProjectError --> ProjectSetup: Edit settings
    Complete --> AnalyzingTargets: Re-analyze
```

```mermaid
sequenceDiagram
    participant U as User
    participant UI as MainWindow
    participant C as Controller
    participant W as AnalysisWorker
    participant S as AnalysisService
    participant I as Infrastructure
    U->>UI: Choose project + Analyze
    UI->>C: start(request)
    C->>W: queued start
    W->>S: run(request)
    S->>I: configure baseline/unity + File API
    loop every buildable C/C++ target
        S->>I: baseline target build
        S->>I: unity target build
        S->>I: probes when needed
        S-->>C: progress/target/case/log event
        C-->>UI: update models/details
    end
    S-->>UI: complete session
```

## 错误处理与恢复

| 失败点 | UI 行为 | 后端行为 | 恢复 |
|---|---|---|---|
| 无 CMakeLists | 设置页行内错误 | 不启动 worker | 重新选择 |
| configure 失败 | 项目错误卡片+日志 | 保存 session | 修改 kit/参数重试 |
| 某 target 普通失败 | target 标记 Baseline Failed | 跳过该 target Unity，继续其他 target | 修复后重新分析 target |
| Unity 失败 | 创建案件 | 最小化/分类 | 复制建议后重新分析 |
| 探针不重放 | 显示证据不足 | `NON_REPLAYABLE` | 查看原日志/排除 source |
| 取消/关闭 | 显示 Cancelling | terminate→kill 超时、保存 | 恢复或新会话 |

## 非功能设计

- 性能：目录扫描在 worker；UI 事件节流到不高于 20Hz；日志视图保留最近 10,000 行，完整日志落盘。
- 安全：源码树只读；work root 重叠校验；QProcess argv；不记录环境变量值。
- 可观测性：每条命令 argv/cwd/exit/duration/log；每 target 阶段和进度事件。
- 可访问性：键盘可达、按钮有 accessibleName、状态不只依赖颜色、支持系统深浅色。
- 视觉适配：`UiTheme` 只依赖 `QPalette` 生成页面样式；系统 Palette 变化时重新应用，不在页面样式中引入固定背景图片或固定浅色背景。bundle 应用图标是独立交付资源。
- 兼容性：应用自身 Qt 5.15/Qt 6.4 双构建；目标项目 Qt 5/6/无 Qt 均通过外部工具处理。

## 复杂度预算与演进规则

| 维度 | 触发条件 | 触发后动作 | 验证 |
|---|---|---|---|
| MainWindow | 同时承担页面构造、分析编排或进程 I/O | 拆 widget/controller/adapter | review/依赖测试 |
| Widget | 超过一个独立页面或同时拥有数据事实 | 拆子 widget/model | QtTest |
| UI theme | 样式逻辑进入页面业务代码或超过单一主题职责 | 抽离/拆分 theme helper | QtTest + 深浅色截图 |
| C++ 文件 | 超过 300 行且有两个变化原因 | 拆分类/职责 | inspect_structure |
| application | 引用 Qt Widgets/QProcess | 提取 port 或移 adapter | include contract |
| 顶层 CMake | 出现模块 source 列表/部署细节 | 下沉子目录/cmake module | CMake review |

## 正确性属性

### PROP-001：全 target 完整性
- 来源：REQ-002。
- 属性：未取消且 configure 成功的 session 中，每个 buildable C/C++ target 恰有一个终态结果。
- 验证：多 target 夹具。

### PROP-002：基线门禁
- 来源：REQ-004。
- 属性：普通构建失败的 target 探针数始终为 0，但不阻断后续 target。
- 验证：混合 target 端到端测试。

### PROP-003：1-minimal
- 来源：REQ-005。
- 属性：标记 MINIMIZED 的有序集合可复现目标指纹，移除任一元素后不可复现。
- 验证：domain 性质测试。

### PROP-004：UI 线程安全
- 来源：REQ-003、REQ-008。
- 属性：所有 QWidget 更新发生在 GUI thread，所有外部命令不在 GUI thread。
- 验证：QtTest thread assertion。

### PROP-005：源码树不变
- 来源：REQ-007。
- 属性：完成、失败、取消、超时前后源码内容/权限/时间戳不变。
- 验证：快照测试。

### PROP-006：报告一致性
- 来源：REQ-006、REQ-007。
- 属性：界面、JSON、Markdown、CMake 引用相同 session/case/source ID。
- 验证：导出契约测试。

### PROP-007：取消终止
- 来源：REQ-003。
- 属性：取消后不再启动新 target，当前 QProcess 在超时边界内终止，session 为 CANCELLED/PARTIAL。
- 验证：长运行假进程 QtTest。

## 测试策略

- domain 使用纯 C++ 单元/性质测试。
- application 使用 fake ports 验证全 target 状态机、门禁和取消。
- infrastructure 使用真实 CMake/Clang 夹具。
- UI 使用 QtTest offscreen 验证交互、线程和 model/view。
- delivery 使用 bundle 结构、图标元数据/多尺寸表示、依赖和启动验证。

## 需求覆盖矩阵

| 行为 | 组件 | ARCH/BUILD | 决策 | 属性 | 验证 |
|---|---|---|---|---|---|
| REQ-001 | SetupWidget/Controller | ARCH-004/BUILD-002 | DEC-001,005 | PROP-004 | QtTest |
| REQ-002 | Inspector/Service/Dashboard | ARCH-002,003,004 | DEC-002 | PROP-001 | multi-target e2e |
| REQ-003 | Worker/Controller | ARCH-002,006 | DEC-003 | PROP-004,007 | cancel/thread test |
| REQ-004 | BuildRunner/Service | ARCH-002,003 | DEC-002,004 | PROP-002,005 | CMake fixtures |
| REQ-005 | domain/IssueDetail | ARCH-001,004 | DEC-005 | PROP-003 | domain+UI test |
| REQ-006 | Suggestion/Exporter | ARCH-001,003,004 | DEC-006 | PROP-006 | export test |
| REQ-007 | SessionStore | ARCH-003,007 | DEC-004,006 | PROP-005,006 | recovery test |
| REQ-008 | all UI | ARCH-004,006 | DEC-003,005 | PROP-004 | offscreen UI test |
| REQ-009 | deployment | BUILD-003,005 | DEC-001,007 | PROP-005 | bundle/icon/smoke |

## 风险与未决问题

- RISK-001：大型项目逐 target 双构建耗时高；提供过滤、取消、缓存和进度，不牺牲完整性声明。
- RISK-002：目标项目自定义 wrapper/response file 可能无法重放；明确 `NON_REPLAYABLE`。
- RISK-003：真实 Qt 5/6 项目 AUTOGEN 差异需要后续用户项目校准。
- [x] GUI 使用 Qt 5.15.2/Qt 6.4.3 Widgets 共源码构建，目标项目 Qt 版本运行时探测。
- [x] required 交付平台为 macOS arm64 `.app`；Windows/Linux 后续。
