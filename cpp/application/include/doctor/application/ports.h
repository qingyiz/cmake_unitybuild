#pragma once

#include "doctor/domain/models.h"

#include <atomic>
#include <functional>
#include <string>
#include <vector>

namespace doctor::application {

struct ProjectInventory {
    std::vector<doctor::domain::TargetResult> targets;
    std::string baselineBuildDirectory;
    std::string unityBuildDirectory;
    std::string error;
    bool valid{false};
};

using LogSink = std::function<void(const std::string&)>;

class IProjectInspector {
public:
    virtual ~IProjectInspector() = default;
    virtual ProjectInventory inspect(
        const doctor::domain::ProjectConfig& config,
        std::atomic_bool& cancelled,
        const LogSink& log) = 0;
};

class ITargetAnalyzer {
public:
    virtual ~ITargetAnalyzer() = default;
    virtual doctor::domain::TargetResult analyze(
        const doctor::domain::ProjectConfig& config,
        const ProjectInventory& inventory,
        const doctor::domain::TargetResult& target,
        std::atomic_bool& cancelled,
        const LogSink& log) = 0;
};

}  // namespace doctor::application
