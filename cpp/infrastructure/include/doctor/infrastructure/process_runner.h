#pragma once

#include <QString>
#include <QStringList>

#include <atomic>
#include <functional>

namespace doctor::infrastructure {

struct ProcessResult {
    int exitCode{-1};
    QString output;
    bool timedOut{false};
    bool cancelled{false};
};

class ProcessRunner {
public:
    ProcessResult run(
        const QString& executable,
        const QStringList& arguments,
        const QString& workingDirectory,
        int timeoutSeconds,
        std::atomic_bool& cancelled,
        const std::function<void(const QString&)>& log) const;
};

}  // namespace doctor::infrastructure
