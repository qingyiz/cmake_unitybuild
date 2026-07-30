#pragma once

#include "doctor/domain/models.h"

#include <QObject>
#include <QVariantMap>

#include <atomic>

class QThread;

namespace doctor::ui {

class AnalysisWorker;

class AnalysisController final : public QObject {
    Q_OBJECT

public:
    explicit AnalysisController(QObject* parent = nullptr);
    ~AnalysisController() override;

    void start(const doctor::domain::ProjectConfig& config);
    void cancel();
    bool isRunning() const;
    bool exportReport(const QString& destinationDirectory, QString* error) const;

signals:
    void progressChanged(
        const QString& stage,
        const QString& target,
        const QString& message,
        int completed,
        int total);
    void logReceived(const QString& text);
    void targetReceived(const QVariantMap& target);
    void analysisFinished(bool cancelled);

private:
    QThread* thread_{nullptr};
    AnalysisWorker* worker_{nullptr};
    QString workDirectory_;
};

}  // namespace doctor::ui
