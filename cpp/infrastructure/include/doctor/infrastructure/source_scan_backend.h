#pragma once

#include "doctor/application/ports.h"

namespace doctor::infrastructure {

class SourceScanBackend final : public doctor::application::ISourceScanner {
public:
    doctor::application::ProjectInventory scan(
        const doctor::domain::ProjectConfig& config,
        std::atomic_bool& cancelled,
        const doctor::application::LogSink& log) override;
};

}  // namespace doctor::infrastructure
