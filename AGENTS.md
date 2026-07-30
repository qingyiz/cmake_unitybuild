# AGENTS.md

<!-- BEGIN CODEX SPEC -->
## 当前 Codex Spec

后续 AI 在分析、设计、编码、测试或审查前，必须按顺序读取并遵循当前 Spec：`unity-build-doctor-gui`。

1. 需求文档：`.codex/specs/unity-build-doctor-gui/requirements.md`
2. 设计文档：`.codex/specs/unity-build-doctor-gui/design.md`
3. 任务文档：`.codex/specs/unity-build-doctor-gui/tasks.md`

执行规则：

- 开始任务前，定位对应的需求/验收标准、`ARCH-*`/`BUILD-*` 约束和 `TASK-*`。
- 按 `tasks.md` 的依赖顺序与修改范围实施；未经更新 Spec，不扩大任务边界。
- 实现与文档冲突时先停止编码，更新受影响的上游文档、追踪关系和任务状态。
- 完成任务后回写实施记录与验证证据，保持代码、测试和三份 Spec 文档一致。
<!-- END CODEX SPEC -->
