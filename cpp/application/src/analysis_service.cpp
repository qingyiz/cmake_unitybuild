#include "doctor/application/analysis_service.h"

#include <algorithm>

namespace doctor::application {

ProjectAnalysisService::ProjectAnalysisService(
    IProjectInspector& inspector,
    ITargetAnalyzer& analyzer)
    : inspector_(inspector), analyzer_(analyzer) {}

doctor::domain::ProjectSession ProjectAnalysisService::run(
    const doctor::domain::ProjectConfig& config,
    std::atomic_bool& cancelled,
    const EventSink& events,
    const TargetSink& targets,
    const LogSink& logs) {
    doctor::domain::ProjectSession session;
    session.sourceDirectory = config.sourceDirectory;
    session.workDirectory = config.workDirectory;
    events({"Inspecting", {}, "正在配置项目并读取 CMake File API…", 0, 0});
    auto inventory = inspector_.inspect(config, cancelled, logs);
    if (!inventory.valid || cancelled.load()) {
        session.cancelled = cancelled.load();
        events(AnalysisEvent{
            QString::fromUtf8(session.cancelled ? "Cancelled" : "ProjectError"),
            {},
            session.cancelled
                ? QString::fromUtf8("分析已取消")
                : QString::fromStdString(inventory.error),
            0,
            static_cast<int>(inventory.targets.size())});
        return session;
    }

    const int total = static_cast<int>(inventory.targets.size());
    int completed = 0;
    for (const auto& pending : inventory.targets) {
        if (cancelled.load()) {
            break;
        }
        events(AnalysisEvent{
            QStringLiteral("Analyzing"),
            QString::fromStdString(pending.name),
            QString::fromUtf8("正在分析 ") + QString::fromStdString(pending.name),
            completed,
            total});
        auto result = analyzer_.analyze(
            config, inventory, pending, cancelled, logs);
        session.targets.push_back(result);
        ++completed;
        targets(result);
        events(AnalysisEvent{
            QStringLiteral("Analyzing"),
            QString::fromStdString(pending.name),
            QString::fromUtf8("已完成 ") + QString::fromStdString(pending.name),
            completed,
            total});
    }
    session.cancelled = cancelled.load();
    events(AnalysisEvent{
        QString::fromUtf8(session.cancelled ? "Cancelled" : "Complete"),
        {},
        QString::fromUtf8(
            session.cancelled ? "分析已取消，部分结果已保留" : "整个项目分析完成"),
        completed,
        total});
    return session;
}

}  // namespace doctor::application
