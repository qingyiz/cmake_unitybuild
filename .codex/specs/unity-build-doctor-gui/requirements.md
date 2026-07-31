# 需求文档：Unity Build Doctor GUI

> 阶段：requirements
>
> 工作流：design-first
>
> 状态：已起草
>
> 最近更新：2026-07-30

## 事实与环境基线

| ID | 事实或未知项 | 状态 | 证据/来源 | 对需求的影响 |
|---|---|---|---|---|
| FACT-001 | 用户要求有界面、使用 Qt+C++，选择一个项目后分析整个项目。 | 用户明确 | 本次用户反馈 | GUI、Qt/C++、全项目是 Must |
| FACT-002 | 本机 Qt 6.4.3 macOS Widgets/Test/macdeployqt 可用，Framework 为 universal。 | 已验证 | qtpaths、CMake configs、file | 可构建/测试/部署 Qt 6 `.app` |
| FACT-003 | 本机 Qt 5.15.2 macOS Widgets/Test/macdeployqt 可用，Framework 为 arm64。 | 已验证 | qmake、CMake configs、file | 同一应用应可使用 Qt 5.15 或 Qt 6.4 构建 |
| FACT-004 | 当前 Python CLI 已完成但不满足产品形态。 | 已验证 | 当前代码与用户反馈 | 保留参考，不作为运行时 |
| FACT-005 | 当前 required 原生环境为 macOS arm64。 | 已验证 | Clang target | 首个交付为 `.app` |
| FACT-006 | 用户提供原生界面截图并明确要求美化；当前页面控件过度横向拉伸、主操作和分组层级不足。 | 用户明确/已观察 | 2026-07-29 用户截图 | 设置页与工作台需要一致的视觉系统和更清晰的信息层级 |
| FACT-007 | 用户要求使用 `CMakePresets.json` 设置，并支持 Qt 5/Qt 6。 | 用户明确 | 2026-07-30 用户反馈 | 构建入口、缓存路径、测试矩阵和源码 API 必须双版本兼容 |
| FACT-008 | 当前开发包与部署包的 `CFBundleIconFile` 为空，Resources 中没有 `.icns`；用户要求应用显示图标。 | 用户明确/已验证 | 2026-07-30 用户反馈、`plutil`、bundle tree | macOS bundle 必须新增自有图标资源并验证 Finder/Dock 使用的 bundle 元数据 |
| FACT-009 | 用户要求增加无需 configure/build 的源码扫描模式，首批检查文件级宏定义、文件作用域 `using namespace` 与文件作用域同名 `static`。 | 用户明确 | 2026-07-30 用户反馈 | 必须新增与构建验证并列的只读分析模式，并明确静态风险不等于已确认编译错误 |
| FACT-010 | 用户在 1920×1280 工作台截图中指出问题详情可见区域太小，不方便阅读多条风险和 CMake 建议。 | 用户明确/已观察 | 2026-07-30 用户截图 | 工作台必须允许扩大详情区域，且不能只能依靠固定左右/上下比例 |

## 问题与目标

### 问题陈述

开发者不希望通过终端组织大量参数、查找报告文件或逐 target 重跑。他需要在一个桌面应用里选择项目、检查工具链、启动分析、持续看到整个项目的 target 进度，并在问题列表中直接查看冲突文件、证据和可复制建议。

### 角色

| 角色 | 目标 | 限制 |
|---|---|---|
| C++/Qt 开发者 | 用 GUI 分析完整项目 Unity 兼容性 | 不要求掌握工具命令 |
| 构建维护者 | 查看 target 级结果、日志和可导出建议 | 决定是否手工应用 |

### 范围

- 范围内：应用自身使用 Qt 5.15/Qt 6.4 构建、CMake C/C++ 项目的构建验证、任意目录内 C/C++ 源文件的只读快速扫描、Qt 5/6/无 Qt 目标项目、全 target 分析、GUI、导出。
- 范围外：直接分析仅 qmake 工程、自动修改源码/CMake、云上传、IDE 插件、签名/公证。

## 用户旅程

1. 打开 `.app`，选择或拖入项目目录。
2. 用户选择“源码快速扫描”或“构建验证”；只有构建验证检查 CMakeLists、CMake executable、generator 与构建参数。
3. 用户点击“开始分析”，进入工作台。
4. 源码模式逐项展示静态风险；构建模式发现全部 target 并展示 baseline/Unity/探针阶段。
5. 用户可筛选失败 target/问题类别，查看最小冲突集合、证据和日志。
6. 用户复制建议或导出报告，手工修改项目。
7. 用户返回应用重新分析全部或选中 target，确认问题变化。

## 功能需求

### REQ-001：图形化项目配置

**用户故事：** 作为开发者，我希望在界面选择项目和工具链，从而不使用终端。

#### 验收标准

- AC-001.1：应用启动时应显示项目设置页，并支持文件夹选择和拖放源码目录。
- AC-001.2：选择目录后应验证 `CMakeLists.txt`、源码/工作目录不重叠、CMake executable 和 generator。
- AC-001.3：界面应允许配置 configuration、target 过滤、并行数、探针预算、超时和重复的 CMake `-D` 参数。
- AC-001.4：无效配置应在设置页就地显示，且不得启动分析 worker。
- AC-001.5：应用应记住最近一次非敏感设置和最近项目。

### REQ-002：发现并分析整个项目

**用户故事：** 作为开发者，我希望应用自动遍历全部 target，从而获得项目级结论。

#### 验收标准

- AC-002.1：configure 成功后，系统应从 CMake File API 枚举全部 buildable C/C++ library/executable target。
- AC-002.2：未启用过滤时，每个 target 应独立获得终态：通过、普通失败、Unity 失败、不可分析、取消。
- AC-002.3：一个 target 失败时，系统应继续分析其余 target。
- AC-002.4：工作台应显示 target 总数、完成数、问题数和各状态计数。
- AC-002.5：用户应能仅重新分析选中 target。

### REQ-003：后台进度与取消

**用户故事：** 作为开发者，我希望长时间分析时界面仍可操作并可取消。

#### 验收标准

- AC-003.1：目录扫描、CMake、编译、探针和最小化不得阻塞 GUI thread。
- AC-003.2：界面应显示当前阶段、target、总体进度、已用时间和实时日志。
- AC-003.3：点击取消后应停止启动新 target，并在超时内 terminate/kill 当前外部进程。
- AC-003.4：取消后应保存部分结果，允许恢复或重新分析。
- AC-003.5：关闭窗口时若任务运行，应执行受控取消并安全退出 worker thread。

### REQ-004：区分迁移问题与 Unity 问题

**用户故事：** 作为构建维护者，我希望每个 target 先验证普通构建，从而避免错误归因。

#### 验收标准

- AC-004.1：每个 target 应先运行 Unity OFF 的普通构建，再决定是否运行 Unity ON。
- AC-004.2：普通构建失败的 target 应标记 `BASELINE_FAILED`，探针数为 0，并显示原始日志。
- AC-004.3：普通构建通过、Unity 失败时，应记录失败 Unity driver、有序 source 和失败指纹。
- AC-004.4：configure/link/codegen/timeout 应显示真实失败阶段，不统一显示为重定义。

### REQ-005：问题定位与详情

**用户故事：** 作为开发者，我希望在界面直接看到冲突文件和根因证据。

#### 验收标准

- AC-005.1：可重放失败应输出保持顺序的 1-minimal source 集合和探针次数。
- AC-005.2：系统应至少分类：文件级 static、匿名命名空间、宏泄漏、include 顺序、头文件定义、逐文件选项、Qt/生成源、单文件、未知。
- AC-005.3：问题详情应显示 target、阶段、指纹、最小文件、证据、置信度和候选解释。
- AC-005.4：用户应能按 target、状态、分类和严重度筛选，并打开源文件所在目录/日志目录。
- AC-005.5：证据不足时应显示 `NON_REPLAYABLE/UNKNOWN`，不得声称已确认根因。

### REQ-006：修复建议与导出

**用户故事：** 作为开发者，我希望从界面复制建议并导出报告。

#### 验收标准

- AC-006.1：系统应提供 source 排除、减小 batch、显式 group、target 关闭和适用的源码建议。
- AC-006.2：建议应按目标 CMake 版本门控，并说明风险、收益和放置作用域。
- AC-006.3：用户应能一键复制单条 CMake 片段或完整建议。
- AC-006.4：用户应能选择目录导出一致的 JSON、Markdown、CMake 文件。
- AC-006.5：应用不得自动写入目标项目。

### REQ-007：会话、安全与隐私

**用户故事：** 作为开发者，我希望分析可恢复且不会污染项目。

#### 验收标准

- AC-007.1：所有 build/probe/session/report 应位于源码树外。
- AC-007.2：完成、失败、取消和超时前后源码内容、权限和时间戳应保持不变。
- AC-007.3：session 应原子保存，并在应用重启后可打开最近分析。
- AC-007.4：源码、编译命令或工具链变化时，相关探针缓存应失效。
- AC-007.5：报告不应收集环境变量值、凭据或源码全文。

### REQ-008：桌面交互质量

**用户故事：** 作为桌面用户，我希望工作台清晰、可访问且能处理大项目。

#### 验收标准

- AC-008.1：窗口最小尺寸下主要操作仍可达，详情和日志可折叠。
- AC-008.2：状态不得只靠颜色表达，并应提供文本/图标。
- AC-008.3：主要控件应支持键盘导航和 accessibleName。
- AC-008.4：10,000 行以上日志不得导致 UI 明显冻结；完整日志仍保存在文件。
- AC-008.5：应用应跟随系统深浅色，不硬编码低对比度文本。
- AC-008.6：在 1320×840 参考窗口和 1080×720 最小窗口下，设置页应显示明确的页头、卡片分组和唯一主操作；主要文本不得截断，输入区域不得无边界横向拉伸。
- AC-008.7：工作台的问题详情与 CMake 建议区域应支持拖动调整上下比例，并提供可逆的“专注详情”操作；进入专注状态后应隐藏结果列表和实时日志，让详情占据主要内容区域，退出后恢复列表、日志和可调整分栏。

### REQ-009：macOS 原生交付

**用户故事：** 作为用户，我希望双击应用即可使用。

#### 验收标准

- AC-009.1：开发构建应生成 `build/bin/UnityBuildDoctor.app`。
- AC-009.2：部署应生成 `dist/UnityBuildDoctor.app`，包含 Qt frameworks 和 cocoa platform plugin。
- AC-009.3：部署应用应在不依赖 Qt 安装绝对路径时启动并显示主窗口。
- AC-009.4：本阶段仅要求无需身份凭据的本地 ad-hoc 签名，不要求 Developer ID 签名、公证或 dmg/pkg，并应在文档明确。
- AC-009.5：仓库应提供可移植的 `CMakePresets.json`，至少包含 Qt 5/Qt 6 的 Debug configure、build 和 test presets；用户本机绝对 Qt 路径仅允许放在环境变量或被忽略的 `CMakeUserPresets.json`。
- AC-009.6：Qt 5.15.2 与 Qt 6.4.3 presets 应从相同源码分别生成 `.app` 并通过同一组 CTest；配置时应拒绝与 preset 请求主版本不一致的 Qt。
- AC-009.7：Qt 5/Qt 6 开发 `.app` 与 `dist/UnityBuildDoctor.app` 的 `Info.plist` 应声明非空 `CFBundleIconFile`，且对应 `.icns` 应位于 `Contents/Resources` 并包含 16–1024 px 的 macOS 图标表示。
- AC-009.8：在 macOS 上执行普通 `cmake --build` 后，build tree 中的 `UnityBuildDoctor.app` 应已包含当前 Qt Kit 的 frameworks、cocoa platform plugin 并通过本地 ad-hoc 签名校验；用户不得需要再运行部署脚本才能复制并启动该 `.app`。

### REQ-010：无需构建的源码快速扫描

**用户故事：** 作为尚未准备好 CMake 工具链的开发者，我希望只选择源码目录就得到 Unity Build 风险清单。

#### 验收标准

- AC-010.1：设置页应允许在“源码快速扫描”和“构建验证”之间选择，且默认保持现有构建验证行为。
- AC-010.2：在源码快速扫描模式下，只要项目目录可读并包含受支持的 C/C++ 源文件，系统就不得要求 `CMakeLists.txt`、CMake executable、generator 或成功构建。
- AC-010.3：源码扫描应递归读取 `.c/.cc/.cpp/.cxx/.m/.mm`，跳过符号链接、隐藏版本控制目录、常见 build/output 目录及大于 2 MiB 的单文件，并在后台线程中支持取消。
- AC-010.4：宏规则应报告源文件结束时仍有效的 `#define`，并对不同文件中同名但替换内容不同的活动宏给出更高风险；已由 `#undef` 清理的宏不得报告。
- AC-010.5：`using namespace` 规则应只报告源文件/全局作用域声明；函数体、类体、注释和字符串中的文本不得报告。
- AC-010.6：`static` 规则应报告不同源文件中同名的文件作用域静态函数或变量；函数局部 static、类静态成员、注释和字符串不得报告。
- AC-010.7：每个发现应包含稳定规则编号、风险等级、置信度、文件与行号、证据和只读修复建议；界面、session 与导出报告应明确标记 `source-scan`，不得把静态候选写成已确认 Unity 编译失败。
- AC-010.8：源码快速扫描不得启动 CMake、编译器或其他外部构建进程，也不得修改被扫描目录的内容、权限或时间戳。

## 非功能需求

| ID | 类别 | 可测约束 | 测量 |
|---|---|---|---|
| NFR-001 | 响应性 | 外部命令运行时 GUI 事件循环持续响应；事件刷新≤20Hz | QtTest heartbeat |
| NFR-002 | 安全 | 目标源码零写入 | 快照测试 |
| NFR-003 | 可靠性 | 每个 target 终态唯一；取消后不启动新 target | 状态机测试 |
| NFR-004 | 诊断质量 | 8 类夹具定位 100%，分类≥90% | C++ e2e |
| NFR-005 | 容量 | 500 targets、10,000 issues 的 model 操作不复制源码全文 | model benchmark |
| NFR-006 | 可部署 | build-tree 与 deployed `.app` 均无开发机 Qt 绝对依赖且包含 cocoa plugin | delivery check |
| NFR-007 | 构建兼容 | Qt 5.15 与 Qt 6.4 共用源码和 target 图，不使用版本分支复制页面/后端 | dual-preset build + CTest |
| NFR-008 | 扫描边界 | 源码模式只做进程内只读词法扫描；单文件读取上限 2 MiB，取消检查至少每文件一次 | source-scan fixtures + fake invalid CMake |

## 边界、错误与状态转换

| 场景 | 预期行为 | 关联 ID |
|---|---|---|
| 项目无 CMakeLists | 设置页阻止开始并显示错误 | REQ-001 |
| configure 失败 | 项目级错误、日志可见、不伪造 targets | REQ-002, REQ-004 |
| 单个 target 普通失败 | 标记 baseline failed，继续后续 target | REQ-002, REQ-004 |
| Unity 失败但探针不重放 | 显示 non-replayable 与原证据 | REQ-005 |
| 用户取消 | 停止当前进程、不启动新 target、保存 partial session | REQ-003, REQ-007 |
| 导出失败 | 保留 session，界面显示 I/O 错误 | REQ-006, REQ-007 |
| 源码目录无受支持文件 | 设置页或工作台显示可恢复错误，不启动构建工具 | REQ-010 |
| 扫描命中跨 target 同名符号 | 标记为项目级候选并说明需要构建验证，不声称必然冲突 | REQ-005, REQ-010 |

## 需求分析记录

| ID | 类型 | 涉及需求 | 发现 | 决议 |
|---|---|---|---|---|
| ANA-001 | 范围 | REQ-002 | “整个项目”若只执行 all build 会被首错截断 | 逐 target 分析并继续 |
| ANA-002 | 架构 | REQ-003 | GUI 不能包装同步 CLI | 原生 worker/事件流 |
| ANA-003 | 兼容性 | REQ-001,004 | 分析器 Qt 与目标 Qt 可能不同 | 外部工具解耦 |
| ANA-004 | 安全 | REQ-006,007 | 自动 apply 风险高 | 只复制/导出 |
| ANA-005 | 性能 | REQ-002,003 | 全 target 双构建昂贵 | 过滤、缓存、取消、清晰进度 |
| ANA-006 | 体验 | REQ-008 | 原生默认 Widgets 样式在大窗口中层级弱且行长过大 | 使用由系统 Palette 派生的主题 token、受控内容宽度、卡片和主次按钮层级 |
| ANA-007 | 误报 | REQ-010 | 无 configure 时不能可靠知道 target/Unity group，跨文件同名可能属于不同 target | 结果标记为项目级风险候选，保留置信度并建议使用构建模式验证 |
| ANA-008 | 范围 | REQ-010 | 完整 C++ AST 在没有编译参数时不可靠，首版规则只要求三类明确词法模式 | 使用注释/字符串感知和作用域深度的轻量扫描，不宣称语义完备 |
| ANA-009 | 体验 | REQ-008 | 固定左右分栏与固定高度 CMake 建议会同时压缩多问题详情和代码建议 | 使用水平/垂直 splitter，并提供一键可逆的详情专注状态 |
| ANA-010 | 交付 | REQ-009 | 用户把 build tree 中 `.app` 视为可直接复制运行的完整应用；仅在独立 `dist` 步骤收集 Qt 依赖不满足该预期 | macOS app target 的普通 build 在链接后自动部署当前 Qt Kit 并签名，`dist` 保留为安装/发布目录 |

## 需求追踪

| 需求 | 验收标准 | 成功证据 |
|---|---|---|
| REQ-001 | AC-001.1–001.5 | SetupWidget QtTest |
| REQ-002 | AC-002.1–002.5 | multi-target CMake e2e |
| REQ-003 | AC-003.1–003.5 | thread/cancel/recovery tests |
| REQ-004 | AC-004.1–004.4 | mixed target fixture |
| REQ-005 | AC-005.1–005.5 | domain + issue detail tests |
| REQ-006 | AC-006.1–006.5 | clipboard/export tests |
| REQ-007 | AC-007.1–007.5 | snapshot/session/cache tests |
| REQ-008 | AC-008.1–008.7 | offscreen UI/model tests + macOS 原生截图 |
| REQ-009 | AC-009.1–009.8 | dual-preset build/CTest + build-tree/deployed `.app` icon/delivery/signature/smoke |
| REQ-010 | AC-010.1–010.8 | domain fixtures + source directory integration + offscreen UI |

## 未决问题

- [x] GUI 框架：Qt Widgets/C++17，required 构建矩阵为 Qt 5.15.2 与 Qt 6.4.3。
- [x] required 平台：macOS arm64；其他平台后续。
- [x] Python 原型不作为 GUI 运行时依赖。
