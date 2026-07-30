#pragma once

#include "doctor/domain/models.h"

#include <QString>

namespace doctor::infrastructure {

class ReportExporter {
public:
    bool saveSession(
        const doctor::domain::ProjectSession& session,
        QString* error = nullptr) const;

    bool exportAll(
        const QString& workDirectory,
        const QString& destinationDirectory,
        QString* error = nullptr) const;
};

}  // namespace doctor::infrastructure
