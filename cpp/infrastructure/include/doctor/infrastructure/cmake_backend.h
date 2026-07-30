#pragma once

#include "doctor/application/ports.h"
#include "doctor/infrastructure/process_runner.h"

#include <QJsonObject>

namespace doctor::infrastructure {

class CMakeBackend final
    : public doctor::application::IProjectInspector,
      public doctor::application::ITargetAnalyzer {
public:
    doctor::application::ProjectInventory inspect(
        const doctor::domain::ProjectConfig& config,
        std::atomic_bool& cancelled,
        const doctor::application::LogSink& log) override;

    doctor::domain::TargetResult analyze(
        const doctor::domain::ProjectConfig& config,
        const doctor::application::ProjectInventory& inventory,
        const doctor::domain::TargetResult& target,
        std::atomic_bool& cancelled,
        const doctor::application::LogSink& log) override;

private:
    ProcessRunner runner_;
};

}  // namespace doctor::infrastructure
