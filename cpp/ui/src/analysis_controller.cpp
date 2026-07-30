#include "doctor/ui/analysis_controller.h"

#include "doctor/application/analysis_service.h"
#include "doctor/infrastructure/cmake_backend.h"
#include "doctor/infrastructure/report_exporter.h"
#include "doctor/infrastructure/source_scan_backend.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QThread>

namespace doctor::ui {

class AnalysisWorker final : public QObject {
    Q_OBJECT

public:
    void run(doctor::domain::ProjectConfig config) {
        const auto workDirectory = QString::fromStdString(config.workDirectory);
        QDir().mkpath(workDirectory);
        QFile logFile(QDir(workDirectory).filePath(QStringLiteral("analysis.log")));
        logFile.open(QIODevice::WriteOnly | QIODevice::Text);
        doctor::infrastructure::CMakeBackend backend;
        doctor::infrastructure::SourceScanBackend sourceBackend;
        doctor::application::ProjectAnalysisService service(
            backend, backend, &sourceBackend);
        const auto session = service.run(
            config,
            cancelled_,
            [&](const doctor::application::AnalysisEvent& event) {
                emit progress(
                    event.stage, event.target, event.message,
                    event.completed, event.total);
            },
            [&](const doctor::domain::TargetResult& result) {
                QVariantMap map;
                map.insert("name", QString::fromStdString(result.name));
                map.insert("type", QString::fromStdString(result.type));
                map.insert("status", QString::fromStdString(
                    doctor::domain::toString(result.status)));
                map.insert("stage", QString::fromStdString(result.stage));
                QVariantList issues;
                for (const auto& issue : result.issues) {
                    QVariantMap value;
                    value.insert("id", QString::fromStdString(issue.id));
                    value.insert("ruleId", QString::fromStdString(issue.ruleId));
                    value.insert("category", QString::fromStdString(issue.category));
                    value.insert("severity", QString::fromStdString(issue.severity));
                    value.insert("summary", QString::fromStdString(issue.summary));
                    value.insert("fingerprint", QString::fromStdString(issue.fingerprint));
                    value.insert("confidence", issue.confidence);
                    value.insert("suggestion", QString::fromStdString(issue.suggestion));
                    value.insert("cmake", QString::fromStdString(issue.cmakeSnippet));
                    QStringList sources;
                    for (const auto& source : issue.sources) {
                        sources.push_back(QString::fromStdString(source));
                    }
                    QStringList evidence;
                    for (const auto& item : issue.evidence) {
                        evidence.push_back(QString::fromStdString(item));
                    }
                    value.insert("sources", sources);
                    value.insert("evidence", evidence);
                    issues.push_back(value);
                }
                map.insert("issues", issues);
                emit targetReady(map);
            },
            [&](const std::string& text) {
                const auto line = QString::fromStdString(text);
                if (logFile.isOpen()) {
                    logFile.write(line.toUtf8());
                    if (!line.endsWith('\n')) {
                        logFile.write("\n");
                    }
                    logFile.flush();
                }
                emit log(line);
            });
        doctor::infrastructure::ReportExporter exporter;
        QString error;
        if (!exporter.saveSession(session, &error)) {
            emit log(QStringLiteral("保存会话失败：%1").arg(error));
        }
        emit finished(cancelled_.load());
    }

    void requestCancel() { cancelled_.store(true); }

signals:
    void progress(
        const QString& stage,
        const QString& target,
        const QString& message,
        int completed,
        int total);
    void log(const QString& text);
    void targetReady(const QVariantMap& target);
    void finished(bool cancelled);

private:
    std::atomic_bool cancelled_{false};
};

AnalysisController::AnalysisController(QObject* parent)
    : QObject(parent) {}

AnalysisController::~AnalysisController() {
    cancel();
    if (thread_) {
        thread_->quit();
        thread_->wait();
    }
}

void AnalysisController::start(const doctor::domain::ProjectConfig& config) {
    if (isRunning()) {
        return;
    }
    workDirectory_ = QString::fromStdString(config.workDirectory);
    thread_ = new QThread(this);
    worker_ = new AnalysisWorker;
    worker_->moveToThread(thread_);
    connect(thread_, &QThread::started, worker_, [worker = worker_, config] {
        worker->run(config);
    });
    connect(worker_, &AnalysisWorker::progress, this, &AnalysisController::progressChanged);
    connect(worker_, &AnalysisWorker::log, this, &AnalysisController::logReceived);
    connect(worker_, &AnalysisWorker::targetReady, this, &AnalysisController::targetReceived);
    connect(worker_, &AnalysisWorker::finished, this, [this](bool cancelled) {
        emit analysisFinished(cancelled);
        thread_->quit();
    });
    connect(thread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(thread_, &QThread::finished, this, [this] {
        thread_->deleteLater();
        thread_ = nullptr;
        worker_ = nullptr;
    });
    thread_->start();
}

void AnalysisController::cancel() {
    if (worker_) {
        worker_->requestCancel();
    }
}

bool AnalysisController::isRunning() const {
    return thread_ && thread_->isRunning();
}

bool AnalysisController::exportReport(
    const QString& destinationDirectory,
    QString* error) const {
    doctor::infrastructure::ReportExporter exporter;
    return exporter.exportAll(workDirectory_, destinationDirectory, error);
}

}  // namespace doctor::ui

#include "analysis_controller.moc"
