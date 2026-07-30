#pragma once

#include "doctor/application/analysis_event.h"
#include "doctor/application/ports.h"

#include <functional>

namespace doctor::application {

using EventSink = std::function<void(const AnalysisEvent&)>;
using TargetSink = std::function<void(const doctor::domain::TargetResult&)>;

class ProjectAnalysisService {
public:
    ProjectAnalysisService(IProjectInspector& inspector, ITargetAnalyzer& analyzer);

    doctor::domain::ProjectSession run(
        const doctor::domain::ProjectConfig& config,
        std::atomic_bool& cancelled,
        const EventSink& events,
        const TargetSink& targets,
        const LogSink& logs);

private:
    IProjectInspector& inspector_;
    ITargetAnalyzer& analyzer_;
};

}  // namespace doctor::application
