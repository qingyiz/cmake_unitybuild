#include "doctor/infrastructure/process_runner.h"

#include <QElapsedTimer>
#include <QProcess>

namespace doctor::infrastructure {

ProcessResult ProcessRunner::run(
    const QString& executable,
    const QStringList& arguments,
    const QString& workingDirectory,
    int timeoutSeconds,
    std::atomic_bool& cancelled,
    const std::function<void(const QString&)>& log) const {
    ProcessResult result;
    QProcess process;
    process.setProgram(executable);
    process.setArguments(arguments);
    process.setWorkingDirectory(workingDirectory);
    process.setProcessChannelMode(QProcess::MergedChannels);
    log(QStringLiteral("$ %1 %2").arg(executable, arguments.join(' ')));
    process.start();
    if (!process.waitForStarted(5000)) {
        result.output = process.errorString();
        log(result.output);
        return result;
    }
    QElapsedTimer timer;
    timer.start();
    while (process.state() != QProcess::NotRunning) {
        process.waitForReadyRead(50);
        const auto chunk = QString::fromUtf8(process.readAll());
        if (!chunk.isEmpty()) {
            result.output += chunk;
            log(chunk);
        }
        if (cancelled.load()) {
            result.cancelled = true;
            process.terminate();
            if (!process.waitForFinished(1500)) {
                process.kill();
                process.waitForFinished(1500);
            }
            break;
        }
        if (timeoutSeconds > 0 && timer.elapsed() > timeoutSeconds * 1000LL) {
            result.timedOut = true;
            process.terminate();
            if (!process.waitForFinished(1500)) {
                process.kill();
                process.waitForFinished(1500);
            }
            break;
        }
    }
    const auto tail = QString::fromUtf8(process.readAll());
    if (!tail.isEmpty()) {
        result.output += tail;
        log(tail);
    }
    result.exitCode = process.exitCode();
    return result;
}

}  // namespace doctor::infrastructure
