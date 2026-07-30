# 实施计划：Unity Build Doctor GUI

> 阶段：tasks
>
> 状态：执行中（核心 GUI MVP 已完成）
>
> 最近更新：2026-07-30

## 执行波次

| 波次 | 任务 |
|---|---|
| 1 | TASK-001 |
| 2 | TASK-002, TASK-003 |
| 3 | TASK-004, TASK-005 |
| 4 | TASK-006, TASK-007 |
| 5 | TASK-008 |
| 6 | TASK-009 |
| 7 | TASK-010 |
| 8 | TASK-011 |
| 9 | TASK-012 |
| 10 | TASK-013 |

## 任务列表

- [x] TASK-001：建立 Qt/C++ 模块骨架与 `.app`
  - 类型：required
  - 需求：REQ-009
  - 设计：DEC-001；ARCH-001–ARCH-005；BUILD-001–BUILD-003
  - 单一变更原因：建立可编译、可启动、依赖方向正确的原生应用骨架。
  - 模块/构建单元：顶层 CMake + `UnityBuildDoctor`
  - 架构约束：ARCH-001–ARCH-005 / BUILD-001–BUILD-003
  - 依赖变化：新增 Qt6 Core/Widgets/Concurrent/Test targets；无 Python 运行时边
  - 平台/交付物：macOS arm64 `build/bin/UnityBuildDoctor.app`
  - 依赖：无
  - 修改范围：顶层/`cpp/**/CMakeLists.txt`、最小 `main.cpp`、Info.plist、smoke test
  - 产出：空壳窗口与模块 targets
  - 验证：configure/build/CTest、bundle 结构、offscreen 启动
  - 实施记录：已完成。Qt 6.4.3 Widgets/C++17 分层 targets 可编译，生成 macOS arm64 `.app`；`doctor_smoke_tests` 通过。

- [x] TASK-002：移植诊断领域模型与算法
  - 类型：required
  - 需求：REQ-004, REQ-005, REQ-006
  - 设计：DEC-002,006；ARCH-001；PROP-002,003,006
  - 单一变更原因：提供无 UI/进程依赖的 C++ 诊断内核。
  - 模块/构建单元：`doctor_domain`
  - 架构约束：ARCH-001 / BUILD-002
  - 依赖变化：仅 C++17 stdlib
  - 平台/交付物：平台无关静态库，不产生独立交付物
  - 依赖：TASK-001
  - 修改范围：`cpp/domain/`、domain tests
  - 产出：models、fingerprint、ddmin、classifier、suggestions
  - 验证：9 类规则、1-minimal 性质、版本建议测试
  - 实施记录：已完成。实现模型、失败指纹、保序 ddmin、分类器与版本门控 CMake 片段；`doctor_domain_tests` 覆盖多文件最小化和主要分类。

- [x] TASK-003：实现 CMake 项目发现与全 target 清单
  - 类型：required
  - 需求：REQ-001, REQ-002
  - 设计：DEC-002,004；ARCH-002,003；BUILD-004；PROP-001
  - 单一变更原因：把项目目录转换为完整 buildable target inventory。
  - 模块/构建单元：`doctor_infrastructure`
  - 架构约束：ARCH-002,003 / BUILD-002,004
  - 依赖变化：infrastructure 实现 `IProjectInspector`
  - 平台/交付物：平台无关逻辑，macOS 原生验证
  - 依赖：TASK-001
  - 修改范围：CMake/QProcess/File API/compile database adapters 与 fixtures
  - 产出：环境探测、configure、codemodel target 枚举
  - 验证：多 target、无 CMakeLists、configure 失败测试
  - 实施记录：已完成。双 configure tree、CMake File API codemodel 和 buildable target 过滤已实现；真实三 target 夹具通过。

- [x] TASK-004：实现 target 双构建、探针与 session
  - 类型：required
  - 需求：REQ-003, REQ-004, REQ-005, REQ-007
  - 设计：DEC-002–004；ARCH-002,003,007；PROP-002,003,005,007
  - 单一变更原因：实现每个 target 的可取消诊断执行。
  - 模块/构建单元：`doctor_application` + `doctor_infrastructure` port 竖切
  - 架构约束：ARCH-002,003,006,007 / BUILD-002,004
  - 依赖变化：application ports -> QProcess adapters
  - 平台/交付物：session/cache/log，不产生独立应用
  - 依赖：TASK-002, TASK-003
  - 修改范围：service/build/probe/session adapters、混合 target fixtures
  - 产出：逐 target baseline/Unity、继续策略、ddmin、取消、原子 session
  - 验证：全 target 完整性、门禁、取消、源码快照
  - 实施记录：已完成核心竖切。逐 target baseline/Unity、失败继续、compile database 重放、保序最小化、取消和原子 session 已实现；缓存失效策略仍列入 TASK-009 扩展验收。

- [x] TASK-005：实现后台 worker 与控制器
  - 类型：required
  - 需求：REQ-003
  - 设计：DEC-003；ARCH-002,004,006；PROP-004,007
  - 单一变更原因：建立 GUI 与分析服务之间的线程安全事件流。
  - 模块/构建单元：`doctor_application` / `UnityBuildDoctor`
  - 架构约束：ARCH-002,004,006 / BUILD-002
  - 依赖变化：UI -> controller -> worker/application；无 UI -> infrastructure 边
  - 平台/交付物：集成入 `.app`
  - 依赖：TASK-004
  - 修改范围：worker/controller/event DTO 与 QtTest
  - 产出：start/progress/log/case/cancel/finish 信号
  - 验证：GUI heartbeat、thread affinity、cancel/close
  - 实施记录：已完成。QThread worker、进度/日志/target/完成信号和原子取消已接通；控制器三 target 集成测试通过。

- [x] TASK-006：实现项目设置页
  - 类型：required
  - 需求：REQ-001, REQ-008
  - 设计：DEC-005；ARCH-004；PROP-004
  - 单一变更原因：让用户完全通过 GUI 配置项目和工具链。
  - 模块/构建单元：`UnityBuildDoctor` UI
  - 架构约束：ARCH-004 / BUILD-002
  - 依赖变化：Widgets -> controller request；无 QProcess
  - 平台/交付物：`build/bin/UnityBuildDoctor.app`
  - 依赖：TASK-005
  - 修改范围：SetupWidget、drag/drop、validation、QSettings、UI tests
  - 产出：可用项目配置页
  - 验证：有效/无效/拖放/键盘/accessibility QtTest
  - 实施记录：已完成核心界面。支持选择/拖放目录、源码与工作树边界校验、工具链/配置/参数/预算/超时及 QSettings；target 过滤界面留待扩展验收。

- [x] TASK-007：实现分析工作台与问题详情
  - 类型：required
  - 需求：REQ-002, REQ-003, REQ-005, REQ-008
  - 设计：DEC-005；ARCH-004,007；PROP-001,004,006
  - 单一变更原因：可视化整个项目状态、问题、证据和日志。
  - 模块/构建单元：`UnityBuildDoctor` UI
  - 架构约束：ARCH-004,007 / BUILD-002
  - 依赖变化：UI models consume session/events；无进程边
  - 平台/交付物：集成入 `.app`
  - 依赖：TASK-005
  - 修改范围：WorkspaceWidget、models/filter/detail/log widgets、tests
  - 产出：master-detail 工作台、进度、筛选、折叠日志
  - 验证：model projection、selection、10k logs、offscreen screenshot/smoke
  - 实施记录：已完成。target master-detail、状态汇总、筛选、问题证据、CMake 复制、10k 可见日志上限及原生截图通过。

- [ ] TASK-008：实现复制、导出和最近会话
  - 类型：required
  - 需求：REQ-006, REQ-007
  - 设计：DEC-006；ARCH-003,004,007；PROP-005,006
  - 单一变更原因：完成建议的安全交付和会话恢复。
  - 模块/构建单元：`doctor_infrastructure` + UI export use case
  - 架构约束：ARCH-003,004,007 / BUILD-002,004
  - 依赖变化：ReportExporter/SessionStore ports；UI 调用 use case
  - 平台/交付物：JSON/Markdown/CMake 用户工件
  - 依赖：TASK-006, TASK-007
  - 修改范围：export/session/settings adapters、clipboard/recent UI
  - 产出：复制、导出、打开最近 session
  - 验证：跨工件 ID、隐私、原子恢复测试
  - 实施记录：部分完成。原子 `session.json` 与一致 JSON/Markdown/CMake 导出已实现；“打开最近 session”尚未实现，因此任务保持未完成。

- [ ] TASK-009：完成全项目端到端验证
  - 类型：required
  - 需求：REQ-001–REQ-008
  - 设计：全部 DEC/ARCH；BUILD-004；PROP-001–007
  - 单一变更原因：证明真实用户旅程和全 target 语义。
  - 模块/构建单元：全部测试 targets
  - 架构约束：ARCH-001–ARCH-007 / BUILD-001,002,004
  - 依赖变化：仅测试 fixtures
  - 平台/交付物：macOS 原生 test evidence
  - 依赖：TASK-008
  - 修改范围：8 类/多 target fixtures、Qt e2e、README
  - 产出：验收覆盖矩阵和使用说明
  - 验证：CTest 全量、源码快照、cancel、thread、分类指标
  - 实施记录：部分完成。真实 mixed multi-target、后台线程、UI、导出与只读边界已验证；8 类真实工程矩阵、重新分析选中 target、缓存失效仍待补齐。

- [ ] TASK-010：部署并验证 macOS 应用束
  - 类型：required
  - 需求：REQ-009
  - 设计：DEC-001；BUILD-003,005；NFR-006
  - 单一变更原因：交付可双击运行的自包含 `.app`。
  - 模块/构建单元：install/deploy
  - 架构约束：ARCH-005 / BUILD-003,005
  - 依赖变化：macdeployqt 仅部署阶段
  - 平台/交付物：`dist/UnityBuildDoctor.app`
  - 依赖：TASK-009
  - 修改范围：install/deploy scripts、Info.plist、delivery docs
  - 产出：自包含 `.app`
  - 验证：verify_delivery、bundle tree、otool、offscreen/`open` smoke
  - 实施记录：交付物已生成并验证，但依赖 TASK-009 尚未完全关闭，因此任务保持未完成。`dist/UnityBuildDoctor.app` 含 Qt Frameworks/cocoa、使用应用内 RPATH、ad-hoc 签名并隔离启动成功。

- [x] TASK-011：统一设置页与工作台视觉层级
  - 类型：required
  - 需求：REQ-008（AC-008.1–008.6）
  - 设计：DEC-005；ARCH-004；BUILD-002
  - 单一变更原因：根据用户截图改善大窗口下的视觉层级、行长、主次操作和深浅色一致性。
  - 模块/构建单元：`doctor_ui`
  - 架构约束：ARCH-004 / BUILD-002
  - 依赖变化：新增 UI 内部 `UiTheme` helper，仅依赖 Qt Widgets/Gui；不新增跨层依赖
  - 平台/交付物：集成入 `build/bin/UnityBuildDoctor.app` 与 `dist/UnityBuildDoctor.app`
  - 依赖：TASK-006, TASK-007
  - 修改范围：`cpp/ui/` 页面布局与主题、UI tests、原生截图；不修改 domain/application/infrastructure
  - 产出：Palette 自适应主题、受控宽度设置页、卡片分组、主按钮、统一工作台样式
  - 验证：QtTest、1080×720/1320×840 截图、浅色/深色 Palette 样式测试、CTest、`.app` 启动
  - 实施记录：已完成。新增 `UiTheme` 从运行时 `QPalette` 派生浅色/深色 token；设置页改为受控宽度页头、双卡片和主行动区，工作台统一为页头/进度/结果详情/日志卡片。QtTest 覆盖 1080×720 主操作可见、双卡片层级、浅深 Palette 差异和原有交互；4/4 CTest 通过。macOS 1320×840 设置页与工作台截图完成人工检查；结构复查后 view construction 与状态逻辑已拆分，相关文件均低于 300 行。部署 `.app` 已重新安装、ad-hoc 签名并验证无开发机 Qt 绝对 RPATH。

- [x] TASK-012：引入 CMake Presets 与 Qt 5/Qt 6 双构建矩阵
  - 类型：required
  - 需求：REQ-009（AC-009.5, AC-009.6）、NFR-007
  - 设计：DEC-001；BUILD-001–BUILD-003,005
  - 单一变更原因：让开发者通过标准 presets 从同一源码选择 Qt 5.15 或 Qt 6.4 完成配置、构建和测试。
  - 模块/构建单元：顶层 CMake、全部 Qt-linked targets、Presets
  - 架构约束：ARCH-001–ARCH-005 / BUILD-001–BUILD-003,005
  - 依赖变化：Qt target 名从固定 `Qt6::*` 改为 `Qt${QT_VERSION_MAJOR}::*`；不新增运行时模块
  - 平台/交付物：macOS arm64；`out/build/qt5-debug/bin/UnityBuildDoctor.app`、`out/build/qt6-debug/bin/UnityBuildDoctor.app`
  - 依赖：TASK-001, TASK-011
  - 修改范围：`CMakeLists.txt`、`cpp/**/CMakeLists.txt`、Qt 公共源码兼容点、`CMakePresets.json`、本机 `CMakeUserPresets.json`、`.vscode/settings.json`、README、dual-build tests
  - 产出：Qt 主版本 gate、便携 configure/build/test presets、本机继承 presets、Qt5/Qt6 共源码构建
  - 验证：列出 presets；Qt5/Qt6 configure/build/CTest；两个 `.app` 路径、Mach-O、offscreen smoke；错误主版本 gate
  - 实施记录：已完成。`CMakePresets.json`（schema v3，兼容 CMake 3.21）
    提供 Qt 5/Qt 6 的 Debug configure/build/test 与 Release build presets，
    本机路径由环境变量或已忽略的 `CMakeUserPresets.json` 提供；VS Code
    CMake Tools 已切换为强制使用 presets。全部 Qt-linked targets 使用动态
    `Qt${QT_VERSION_MAJOR}::*`，公共 UI 源码修复 Qt 5 不支持的多参数
    `QString::arg` 调用。Qt 5.15.2 与 Qt 6.4.3 已从同一源码分别完成
    configure/build，均 4/4 CTest 通过；两个 `.app` 均为 macOS arm64
    Mach-O，并实际启动生成 2640×1680 截图。Qt 根目录 gate 已验证会拒绝
    “Qt 5 preset 指向 Qt 6 根目录后回退到其他 Qt 5 安装”的错误配置。

- [x] TASK-013：为 macOS 应用束加入原生图标
  - 类型：required
  - 需求：REQ-009（AC-009.7）
  - 设计：DEC-007；BUILD-002,003,005
  - 单一变更原因：修复 Finder、Dock 与应用切换器显示默认图标的问题。
  - 模块/构建单元：`UnityBuildDoctor` app target 与 macOS bundle resources
  - 架构约束：ARCH-005 / BUILD-002,003,005
  - 依赖变化：新增 bundle 静态资源，不新增运行时库或跨层依赖
  - 平台/交付物：macOS arm64；Qt 5/Qt 6 开发 `.app` 与 `dist/UnityBuildDoctor.app`
  - 依赖：TASK-001, TASK-012
  - 修改范围：图标源 PNG/ICNS、`cpp/app/CMakeLists.txt`、README、bundle/deploy verification
  - 产出：带自有多尺寸图标的开发与部署应用束
  - 验证：`plutil` 检查 `CFBundleIconFile`；Resources 精确路径；`iconutil` 解包并检查 16–1024 px 表示；Qt 5/Qt 6 build/CTest；部署包自包含与启动验证
  - 实施记录：已完成。使用内置图像生成能力制作原创蓝灰色“代码括号 +
    诊断脉冲”1024×1024 透明 PNG，生成十档 16–1024 px
    `UnityBuildDoctor.icns`；`UnityBuildDoctor` target 通过
    `MACOSX_PACKAGE_LOCATION=Resources` 和 `MACOSX_BUNDLE_ICON_FILE`
    将图标写入 Qt 5、Qt 6 开发包与部署包。三个 bundle 的
    `CFBundleIconFile` 均为 `UnityBuildDoctor.icns`，资源 SHA-256 一致；
    `iconutil` 解包验证十档表示完整。Qt 5/Qt 6 均 4/4 CTest 通过，
    `verify_delivery.py` 验证两个开发包和自包含部署包通过，部署包
    `codesign --verify --deep --strict` 与隔离 Cocoa 截图启动通过。

## 覆盖检查

| 行为 | 任务 | 状态 |
|---|---|---|
| REQ-001 | TASK-003,006,009 | 待执行 |
| REQ-002 | TASK-003,004,007,009 | 待执行 |
| REQ-003 | TASK-004,005,007,009 | 待执行 |
| REQ-004 | TASK-002,004,009 | 待执行 |
| REQ-005 | TASK-002,004,007,009 | 待执行 |
| REQ-006 | TASK-002,008,009 | 待执行 |
| REQ-007 | TASK-004,008,009 | 待执行 |
| REQ-008 | TASK-006,007,009,011 | 执行中 |
| REQ-009 | TASK-001,010,012,013 | 执行中 |

## 完成门槛

- [ ] TASK-001–013 required 全部完成
- [ ] AC-001.1–AC-008.6、AC-009.1–AC-009.7 与 PROP-001–007 有证据
- [ ] 8 类与多 target CMake 夹具通过
- [ ] GUI thread/cancel/10k 日志/offscreen UI 测试通过
- [ ] `build/bin/UnityBuildDoctor.app` 和 `dist/UnityBuildDoctor.app` 精确验证
- [ ] deployed app 不依赖 Qt 安装绝对路径并包含 cocoa plugin
- [ ] Windows/Linux、签名、公证、DMG 明确未承诺
