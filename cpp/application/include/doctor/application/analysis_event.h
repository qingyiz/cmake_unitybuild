#pragma once

#include <QString>

namespace doctor::application {

struct AnalysisEvent {
    QString stage;
    QString target;
    QString message;
    int completed{0};
    int total{0};
};

}  // namespace doctor::application
