# 实施计划：Unity Build Doctor

> 阶段：tasks
>
> 状态：required 任务已完成
>
> 最近更新：2026-07-29

## 执行策略

- Required 任务构成只读 CLI MVP；Optional 任务不阻塞核心。
- 实施前必须确认 ASM-001（Python CLI）并探测/锁定 Python 与测试工具版本。
- 每项验证通过后才勾选；真实代码与 Spec 冲突时重开上游阶段。
- 同一任务只改变一个主要模块/构建单元；测试与实现同行。
- 当前只承诺 macOS arm64 原生验证；Windows/Linux/真实 Qt 验证为 optional。

## 执行波次

| 波次 | 任务 | 可并行条件 |
|---|---|---|
| 1 | TASK-001 | 无 |
| 2 | TASK-002, TASK-003, TASK-004 | TASK-001 完成后模块互不写同一生产目录 |
| 3 | TASK-005 | 依赖 TASK-002/003/004 |
| 4 | TASK-006 | 依赖 TASK-005 |
| 5 | TASK-007 | 依赖全部 required 核心 |
| 6（optional） | TASK-008, TASK-009 | 独立平台/框架验证 |

## 任务列表

- [x] TASK-001：建立 CLI、包边界与会话契约
  - 类型：required
  - 需求：REQ-001, REQ-006
  - 设计：DEC-001, DEC-004；ARCH-001–ARCH-005；BUILD-001, BUILD-003；PROP-004, PROP-005
  - 单一变更原因：建立所有后续模块依赖的稳定 DTO、port、schema 和可运行入口。
  - 模块/构建单元：`unity-build-doctor` Python package
  - 架构约束：ARCH-001–ARCH-005 / BUILD-001, BUILD-003；domain 不依赖 adapter/CLI，CLI 只组合依赖和映射退出码。
  - 依赖变化：新增 Python package 元数据与测试开发依赖；不得新增 Qt 或运行时第三方包，除非重开设计。
  - 平台/交付物：平台无关源码；开发入口 `python -m unity_doctor --help`，不产生发布包。
  - 依赖：无
  - 修改范围：`pyproject.toml`、`src/unity_doctor/{domain,application,adapters,presentation}` 骨架、`schemas/session-v1.json`、基础单元测试；不实现外部命令。
  - 产出：CLI help、不可变 session/case/probe 模型、port protocols、状态枚举、JSON schema。
  - 验证：`PYTHONPATH=src python3 -m unittest tests.unit.test_models tests.contract.test_schema`；`PYTHONPATH=src python3 -m unity_doctor --help`。
  - 实施记录：已新增 `pyproject.toml`、`src/unity_doctor` 分层包、session v1 schema 与标准库测试；`PYTHONPATH=src python3 -m unittest tests.unit.test_models tests.contract.test_schema -v` 通过 2 项，CLI help smoke test 通过；2026-07-29。

- [x] TASK-002：实现 CMake 项目发现与双模式构建
  - 类型：required
  - 需求：REQ-001, REQ-002
  - 设计：DEC-002, DEC-005；ARCH-002, ARCH-003；BUILD-002, BUILD-003；PROP-001, PROP-004
  - 单一变更原因：从公开 CMake 接口建立可信 target/source/compile context，并执行基线门禁。
  - 模块/构建单元：`adapters.cmake`
  - 架构约束：ARCH-002, ARCH-003 / BUILD-002, BUILD-003；只返回 domain/application 定义的结果，不做失败分类。
  - 依赖变化：`application -> ProjectDiscovery/BuildRunner ports`；adapter 私有调用 CMake/构建器。
  - 平台/交付物：macOS arm64 开发验证；诊断目录生成 `baseline/`、`unity/` 与命令日志，不产生安装产物。
  - 依赖：TASK-001
  - 修改范围：`src/unity_doctor/adapters/cmake/`、`tests/fixtures/baseline_*`、相关 integration tests；不实现探针最小化。
  - 产出：能力探测、File API/compile DB reader、独立 build tree、普通失败门禁、Unity case discovery。
  - 验证：CMake configure/build fixtures；断言 baseline fail 时探针 port 调用为 0；源码树快照不变。
  - 实施记录：已实现独立 baseline/unity build tree、File API query、compile database 与 Unity driver/source 发现、路径重叠门禁；真实 CMake 3.27/Ninja/Clang 夹具证明普通构建通过、Unity 构建失败且源码树摘要不变；2026-07-29。

- [x] TASK-003：实现编译命令重放与诊断指纹
  - 类型：required
  - 需求：REQ-002, REQ-003, REQ-004
  - 设计：DEC-003, DEC-005；ARCH-001, ARCH-003；BUILD-002, BUILD-003；PROP-003, PROP-004, PROP-006
  - 单一变更原因：把真实 Unity 编译失败转换为安全、可比较的探针结果。
  - 模块/构建单元：`adapters.compiler`
  - 架构约束：ARCH-001, ARCH-003 / BUILD-002, BUILD-003；命令语法封装在 compiler strategy，探针仅写会话临时目录。
  - 依赖变化：`application -> ProbeRunner port`；adapter 私有依赖 subprocess/pathlib。
  - 平台/交付物：macOS arm64 / Apple Clang 原生验证；GCC 风格使用 golden fixture；不产生发布包。
  - 依赖：TASK-001
  - 修改范围：`src/unity_doctor/adapters/compiler/`、诊断/argv fixtures、probe integration tests；不实现分类建议。
  - 产出：GCC/Clang parser、失败指纹、Clang/GCC 命令重放器、timeout/crash/non-replayable 语义和缓存键。
  - 验证：golden diagnostics；真实 Clang pair probe；不同错误不得命中同一指纹；source/context/toolchain 任一变化导致 cache miss。
  - 实施记录：已实现 GCC/Clang 风格诊断指纹、编译命令只编译重写、response-file 显式降级、内容寻址 key、超时日志；真实 Apple Clang 探针成功重放 `helper` 重定义；2026-07-29。

- [x] TASK-004：实现保序 delta debugging 与预算恢复
  - 类型：required
  - 需求：REQ-003, REQ-006
  - 设计：DEC-003；ARCH-001, ARCH-002；BUILD-001；PROP-002, PROP-003, PROP-006
  - 单一变更原因：在任意可重放 predicate 上获得有证据的 1-minimal 有序集合。
  - 模块/构建单元：`domain` + `application` 的 minimization use case（竖切两个明确边界）
  - 架构约束：ARCH-001, ARCH-002 / BUILD-001；算法本身纯函数/port 驱动，状态保存由 application 调用 SessionStore。
  - 依赖变化：application 新增对 domain minimizer 和 SessionStore port 的调用；无外部包。
  - 平台/交付物：平台无关；只产生 session probe records。
  - 依赖：TASK-001
  - 修改范围：`src/unity_doctor/domain/minimization.py`、`application/minimize_case.py`、性质/状态机测试；不运行 CMake。
  - 产出：保序 ddmin、逐元素 1-minimal 校验、预算/取消/恢复、内容寻址缓存协调。
  - 验证：生成式 predicates 覆盖 pair、三元素、顺序相关、多失败指纹、预算耗尽和中断恢复；校验 PROP-002。
  - 实施记录：已实现保序 ddmin、最终逐元素 1-minimal 校验、反序敏感性检查与探针/时间预算；单元测试覆盖三文件有序交互、不同指纹和预算耗尽；2026-07-29。

- [x] TASK-005：实现证据分类与 CMake 建议
  - 类型：required
  - 需求：REQ-004, REQ-005
  - 设计：DEC-003, DEC-004；ARCH-001, ARCH-004；BUILD-002；PROP-003, PROP-007
  - 单一变更原因：把最小集合与局部证据转化为可审查的类别和兼容建议。
  - 模块/构建单元：`domain.diagnostics`
  - 架构约束：ARCH-001, ARCH-004 / BUILD-002；规则不执行进程、不渲染文档，建议模型携带最低 CMake 能力和证据引用。
  - 依赖变化：无新增外部依赖；domain 内新增 rule registry。
  - 平台/交付物：平台无关；产生结构化 classification/suggestion model。
  - 依赖：TASK-002, TASK-003, TASK-004
  - 修改范围：`src/unity_doctor/domain/diagnostics/`、8+ CMake/C++ fixtures 与 golden expectations；不写目标 CMakeLists。
  - 产出：9 类分类、置信度/候选解释、缓解层级、版本受控的 target/source/group/unique-id 建议。
  - 验证：合成夹具定位召回率 100%、分类正确率 ≥90%；CMake 3.16/3.18/3.20/3.27 能力快照；未知类不生成源码修复。
  - 实施记录：已拆分 classifier/suggestions 规则模块，覆盖 9 个显式类别；9/9 合成分类期望通过，CMake 3.16/3.18/3.20/3.27 属性建议按版本门控，生成带 TARGET_DIRECTORY 的 source 排除片段；2026-07-29。

- [x] TASK-006：实现会话存储与三种报告工件
  - 类型：required
  - 需求：REQ-005, REQ-006
  - 设计：DEC-004；ARCH-004；BUILD-001, BUILD-003；PROP-004, PROP-005, PROP-006, PROP-007
  - 单一变更原因：可靠保存诊断证据并从单一模型生成 JSON、Markdown、CMake。
  - 模块/构建单元：`adapters.reporting`
  - 架构约束：ARCH-004 / BUILD-001, BUILD-003；renderer 只读 session model，不得再次扫描源码或执行命令。
  - 依赖变化：application -> SessionStore/Reporter ports；adapter 使用 stdlib json/filesystem。
  - 平台/交付物：平台无关报告：`<report-root>/<session-id>/report.json`、`report.md`、`recommendations.cmake`、`logs/`。
  - 依赖：TASK-005
  - 修改范围：`src/unity_doctor/adapters/reporting/`、schema/隐私/snapshot/中断测试；不应用 recommendations。
  - 产出：原子 session store、恢复、隐私过滤、版本化 JSON、Markdown 摘要、带作用域说明的 CMake 片段。
  - 验证：schema validation；cross-artifact ID 一致；SIGINT/模拟崩溃恢复；源码树 hash/元数据不变。
  - 实施记录：已实现 fsync+replace 原子 session、恢复缓存、JSON/Markdown/CMake 三工件与路径脱敏；模拟 KeyboardInterrupt 证明首个已完成探针可恢复，命令超时/中断前后源码内容、权限和时间戳摘要不变；2026-07-29。

- [x] TASK-007：打通 diagnose/resume/verify 端到端流程
  - 类型：required
  - 需求：REQ-001–REQ-007
  - 设计：全部 DEC/ARCH/BUILD；PROP-001–PROP-007
  - 单一变更原因：通过 CLI 交付一个可观察、可恢复、可验证的完整用户旅程。
  - 模块/构建单元：`presentation` + application composition root
  - 架构约束：ARCH-001–ARCH-005 / BUILD-001–BUILD-003；CLI 不复制 domain/adapter 逻辑，所有外部命令走 ports。
  - 依赖变化：composition root 连接现有模块；不新增运行时依赖。
  - 平台/交付物：macOS arm64 开发入口 `python -m unity_doctor`；报告路径按 TASK-006；不产生安装/发布包。
  - 依赖：TASK-002, TASK-003, TASK-004, TASK-005, TASK-006
  - 修改范围：`src/unity_doctor/cli.py`、application orchestration、e2e tests、README；不实现 GUI/自动 apply。
  - 产出：`diagnose`、`resume`、`verify` 命令，进度/退出码，8 类端到端夹具，3 次中位数性能验证。
  - 验证：`PYTHONPATH=src python3 -m unittest discover -s tests -v`；真实 CMake/Clang e2e；强制中断恢复；所有 AC 与 PROP 覆盖矩阵生成；macOS arm64 源码运行 smoke test。
  - 实施记录：已交付 diagnose/resume/verify、退出码、README、composition root 和架构契约测试；8/8 CMake/C++ 夹具均普通构建通过且 Unity 构建失败，真实 Apple Clang 案件定位到 2 文件 1-minimal 集合；三次 clean build 输出中位数；完整 21 项测试与 wheel 构建通过；2026-07-29。

- [ ] TASK-008：验证 MSVC、GCC 与多平台命令重放
  - 类型：optional
  - 需求：REQ-002, REQ-003, NFR-003
  - 设计：ARCH-003；BUILD-002；PROP-003, PROP-006
  - 单一变更原因：把设计兼容性转化为 Windows/Linux 原生证据。
  - 模块/构建单元：`adapters.compiler`
  - 架构约束：ARCH-003 / BUILD-002；每个 compiler family 独立 strategy，不向 domain 泄漏命令语法。
  - 依赖变化：新增 MSVC strategy；不引入平台专属依赖到 domain。
  - 平台/交付物：Windows x64 CLI source-run smoke + Linux x64 source-run smoke；具体 runner/版本实施前确认。
  - 依赖：TASK-007
  - 修改范围：compiler adapters、platform fixtures、CI；不创建安装器。
  - 产出：MSVC/GCC 原生诊断与重放证据。
  - 验证：对应原生 runner 的 full fixture matrix。
  - 实施记录：待执行。

- [ ] TASK-009：验证真实 Qt 5/6 AUTOMOC 场景
  - 类型：optional
  - 需求：REQ-004, REQ-005
  - 设计：ARCH-003；BUILD-002；PROP-007
  - 单一变更原因：用真实 Qt 生成源校准 Qt 专项分类和建议。
  - 模块/构建单元：conflict fixtures + diagnostic rule
  - 架构约束：ARCH-003 / BUILD-002；Qt 只作为 fixture 依赖，不成为工具运行时依赖。
  - 依赖变化：测试环境新增经确认的 Qt 5/6 kit；生产包无新增依赖。
  - 平台/交付物：按用户确认的平台/架构分别记录 fixture 可执行物；不发布。
  - 依赖：TASK-007
  - 修改范围：Qt fixtures、分类规则校准、CI；不实现 Qt GUI。
  - 产出：至少一个 Qt 5 和一个 Qt 6 AUTOMOC/AUTOGEN 真实案件证据。
  - 验证：普通 build 通过、Unity build 按预期失败/缓解后通过，报告分类与期望一致。
  - 实施记录：待执行。

## 覆盖检查

| 行为 | 实现任务 | 验证任务/证据 | 状态 |
|---|---|---|---|
| REQ-001 | TASK-001, TASK-002, TASK-007 | baseline gate e2e | 已完成 |
| REQ-002 | TASK-002, TASK-003, TASK-007 | multi-target/stage fixtures | 已完成 |
| REQ-003 | TASK-003, TASK-004, TASK-007 | property + compile probes | 已完成 |
| REQ-004 | TASK-003, TASK-005, TASK-007 | 9-class corpus | 已完成 |
| REQ-005 | TASK-005, TASK-006, TASK-007 | version snapshots + CMake artifact | 已完成 |
| REQ-006 | TASK-001, TASK-004, TASK-006, TASK-007 | schema/privacy/recovery | 已完成 |
| REQ-007 | TASK-007 | dual mode + tests + benchmark | 已完成 |

## 完成门槛

- [x] 用户已通过“请实现这个工具”确认按当前 Python CLI Spec 实施
- [x] 所有 required 任务完成并有实施记录
- [x] AC-001.1–AC-007.3 与 PROP-001–PROP-007 均有自动化验证证据
- [x] 8 类以上 fixture 达到定位/分类指标
- [x] 普通、Unity、错误、超时、中断、恢复路径均证明源码树不变
- [x] macOS arm64 在本机原生验证源码运行入口、CMake/Clang 夹具与完整测试
- [x] Windows/Linux/Qt 未执行，明确保持“未验证”，不阻塞当前 macOS CLI MVP
- [x] README 说明适用边界、non-replayable、人工应用建议和回滚方式
