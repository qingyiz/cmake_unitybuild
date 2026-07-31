# 实施计划：Unity Build Doctor GUI

> 阶段：tasks
>
> 状态：执行中（核心 GUI MVP 与源码扫描模式已完成）
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
| 11 | TASK-014 |
| 12 | TASK-015 |
| 13 | TASK-016 |
| 14 | TASK-017 |
| 15 | TASK-018 |

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

- [x] TASK-014：实现三类纯源码风险规则
  - 类型：required
  - 需求：REQ-010（AC-010.4–010.7）
  - 设计：DEC-008；ARCH-001；PROP-009
  - 单一变更原因：建立不依赖 Qt、文件系统或编译器的可测试词法规则内核。
  - 模块/构建单元：`doctor_domain`
  - 架构约束：ARCH-001 / BUILD-002
  - 依赖变化：仅 C++17 标准库，无新增 link 依赖
  - 平台/交付物：平台无关静态库，不产生独立交付物
  - 依赖：TASK-002
  - 修改范围：domain source-scan model/parser、domain tests；不读取目录、不修改 UI
  - 产出：`UBD-MACRO-001/002`、`UBD-USING-001`、`UBD-STATIC-001` 稳定发现
  - 验证：表驱动测试覆盖活动/已清理宏、注释/字符串、全局/局部 using、全局/局部/类 static、确定性顺序
  - 实施记录：已完成。`doctor_domain` 新增独立的源码事实提取与风险归并，
    稳定输出 `UBD-MACRO-001/002`、`UBD-USING-001`、
    `UBD-STATIC-001`；解析器感知注释、字符串和花括号深度，已由
    domain tests 覆盖活动/已 `#undef` 宏、块注释、全局/局部 using、
    文件/函数/类 static 与输入顺序确定性。规则和解析文件均低于 300 行。

- [x] TASK-015：接入只读源码目录扫描用例
  - 类型：required
  - 需求：REQ-010（AC-010.2, AC-010.3, AC-010.8）、REQ-003、REQ-007
  - 设计：DEC-008；ARCH-002,003,006,007；PROP-004,005,008
  - 单一变更原因：把项目目录转换为不启动外部进程的可取消源码检查结果。
  - 模块/构建单元：`doctor_infrastructure` + `doctor_application` source-scan port 竖切
  - 架构约束：ARCH-002,003,006,007 / BUILD-002,004
  - 依赖变化：application 新增 `ISourceScanner`；infrastructure 实现只读适配器并依赖 domain 规则
  - 平台/交付物：平台无关扫描逻辑，集成入 macOS `.app`
  - 依赖：TASK-005, TASK-014
  - 修改范围：analysis mode/model、application dispatch、source scanner adapter、session mode、fixtures/integration tests；不修改页面布局
  - 产出：无 CMakeLists/无有效 CMake executable 也能完成的 `source-scan` session
  - 验证：无 CMake fixture、目录排除/大文件/取消、无外部进程、session/report mode 集成测试
  - 实施记录：已完成。新增 `ISourceScanner` 与
    `SourceScanBackend`，递归读取六类源文件并跳过符号链接、版本控制/
    build/output 目录和超过 2 MiB 的文件；application 按
    `AnalysisMode` 分派，源码模式直接投影三个检查项，不调用 inspector、
    target analyzer 或外部进程。无 CMakeLists 且 CMake 路径无效的 fixture
    可生成 `source-scan` session/report；排除、大文件、取消与模式字段均有
    backend 集成测试。

- [x] TASK-016：在设置页和工作台提供源码扫描模式
  - 类型：required
  - 需求：REQ-010（AC-010.1, AC-010.7）、REQ-008
  - 设计：DEC-005,008；ARCH-004,006,007；PROP-004,008
  - 单一变更原因：让用户从 GUI 选择源码扫描并正确理解风险候选结果。
  - 模块/构建单元：`doctor_ui`
  - 架构约束：ARCH-004,006,007 / BUILD-002
  - 依赖变化：UI 只传递 `AnalysisMode` 并消费既有结果事件；不新增文件/QProcess 依赖
  - 平台/交付物：Qt 5/Qt 6 macOS `.app`
  - 依赖：TASK-006, TASK-007, TASK-015
  - 修改范围：设置页模式选择与条件校验、工作台检查项/多问题展示、controller 组合、UI tests、README；不在 UI 实现扫描规则
  - 产出：可选择“源码快速扫描（无需构建）”并查看/导出全部风险
  - 验证：无 CMake 项目 UI 启动、字段禁用、三检查项投影、多问题详情、Qt5/Qt6 CTest、原生截图
  - 实施记录：已完成。设置页新增“构建验证（CMake）/源码快速扫描
    （无需构建）”选择，源码模式放宽 CMakeLists/CMake 校验并禁用构建参数，
    同步切换主操作说明；工作台使用检查项语义、`Risk Found` 汇总并连续展示
    全部规则、级别、文件、行号证据和建议。controller、session、JSON/Markdown
    报告携带模式与规则编号。Qt 5.15.2、Qt 6.4.3 均 4/4 CTest 通过，Qt 6
    1320×840 源码模式截图已人工复核禁用态与主操作；Qt 6 Release 已重新部署
    到 `dist/UnityBuildDoctor.app`，通过 deep/strict 签名校验并使用 Cocoa
    platform plugin 启动。新增 `examples/unity_risk_demo` 手工验收工程，
    后端测试确认三个检查项全部为 `Risk Found`、共 5 条候选；工程普通构建
    成功运行，Unity Build 稳定复现宏重定义警告与 `fileCache` 重定义错误。

- [x] TASK-017：扩大问题详情可见区域
  - 类型：required
  - 需求：REQ-008（AC-008.7）
  - 设计：DEC-005；ARCH-004；PROP-010
  - 单一变更原因：让多条风险详情和 CMake 建议在工作台中拥有可调且可专注的阅读空间。
  - 模块/构建单元：`doctor_ui`
  - 架构约束：ARCH-004 / BUILD-002
  - 依赖变化：无新增 include/link target 边；仅使用既有 Qt Widgets `QSplitter`
  - 平台/交付物：Qt 5/Qt 6 macOS `.app`
  - 依赖：TASK-007, TASK-016
  - 修改范围：`AnalysisWorkspaceWidget` 布局/状态、UI tests、原生截图；不修改分析规则、session、报告或后端
  - 产出：可拖动的详情/CMake 垂直分栏，以及可逆的“专注详情”工作台状态
  - 验证：QtTest 验证分栏子控件与专注状态往返；Qt5/Qt6 CTest；1320×840 工作台截图；部署 `.app` 启动
  - 实施记录：已完成。工作台默认将约 60% 主区域分配给详情；结果/详情改为
    可拖动水平 splitter，详情正文/CMake 建议改为可拖动垂直 splitter 并取消
    CMake 固定最大高度。新增“专注详情/退出专注”可逆状态，专注时隐藏结果列表
    和日志但保留选择、详情与建议。QtTest 覆盖 splitter 方向/非折叠尺寸、状态
    往返、内容保持与控件可见性；Qt 5.15.2、Qt 6.4.3 均 4/4 CTest 通过。
    1320×840 默认/专注截图完成视觉复核；Qt 6 Release 已重新部署，
    `verify_delivery.py --require-self-contained`、Cocoa 启动均通过。

- [x] TASK-018：让普通 macOS build 生成自包含应用
  - 类型：required
  - 需求：REQ-009（AC-009.8）、NFR-006
  - 设计：DEC-009；BUILD-003,005；PROP-011
  - 单一变更原因：把自包含 Qt 运行时收集从额外部署脚本前移到 `UnityBuildDoctor` target 的普通构建结果。
  - 模块/构建单元：`UnityBuildDoctor`
  - 架构约束：ARCH-005 / BUILD-003,005
  - 依赖变化：不新增 C++ include/link 边；macOS app target 新增对当前 Qt Kit `macdeployqt` 与系统 `codesign` 的构建后工具依赖
  - 平台/交付物：macOS arm64；`build/bin/UnityBuildDoctor.app`、`out/build/qt5-debug/bin/UnityBuildDoctor.app`、`out/build/qt6-debug/bin/UnityBuildDoctor.app`
  - 依赖：TASK-012, TASK-013
  - 修改范围：`cpp/app/CMakeLists.txt`、可复用 macOS 部署 CMake 规则、README、Spec 与交付验证；不修改 C++ 业务/UI/分析逻辑
  - 产出：普通 `cmake --build` 后可直接复制运行的 Qt 5/Qt 6 完整 `.app`
  - 验证：Qt 5/Qt 6 全新 configure/build；`verify_delivery.py --require-self-contained`；bundle tree、`otool -L`、deep/strict 签名与 Cocoa 启动；完整 CTest
  - 实施记录：已完成。新增单一职责的
    `cmake/DeployMacOSBundle.cmake`，由 `UnityBuildDoctor` target 在
    macOS `POST_BUILD` 中定位当前 configure 所选 Qt Kit 的
    `macdeployqt`，就地收集 Frameworks/plugins，再执行 ad-hoc 签名及
    deep/strict 校验；未新增 C++ include/link 依赖。使用全新 Qt 5.15.2
    与 Qt 6.4.3 build tree 以及标准 `qt5-debug`/`qt6-debug` presets
    分别构建成功，标准输出 `.app` 为 25 MiB/91 MiB，均包含
    `Contents/Frameworks` 与
    `Contents/PlugIns/platforms/libqcocoa.dylib`，动态依赖使用
    `@rpath`，`verify_delivery.py --require-self-contained` 和
    `codesign --verify --deep --strict` 通过。两套 CTest 均 4/4 通过，
    两个 build-tree 应用均使用 Cocoa 启动成功。

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
| REQ-008 | TASK-006,007,009,011,017 | 执行中 |
| REQ-009 | TASK-001,010,012,013,018 | 执行中 |
| REQ-010 | TASK-014,015,016 | 已完成 |

## 完成门槛

- [ ] TASK-001–018 required 全部完成
- [ ] AC-001.1–AC-008.7、AC-009.1–AC-009.8、AC-010.1–AC-010.8 与 PROP-001–011 有证据
- [ ] 8 类与多 target CMake 夹具通过
- [ ] GUI thread/cancel/10k 日志/offscreen UI 测试通过
- [ ] `build/bin/UnityBuildDoctor.app`、两个 preset build-tree `.app` 和 `dist/UnityBuildDoctor.app` 精确验证
- [ ] build-tree 与 deployed app 均不依赖 Qt 安装绝对路径并包含 cocoa plugin
- [ ] Windows/Linux、签名、公证、DMG 明确未承诺
