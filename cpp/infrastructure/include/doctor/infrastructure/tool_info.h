#pragma once

#include <QString>

namespace doctor::infrastructure {

struct ToolInfo {
    QString executable;
    QString version;
    bool available{false};
};

}  // namespace doctor::infrastructure
