#pragma once

#include "doctor/domain/models.h"

#include <QWidget>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;

namespace doctor::ui {

class ProjectSetupWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ProjectSetupWidget(QWidget* parent = nullptr);

    doctor::domain::ProjectConfig config() const;
    bool validate(QString* error) const;
    void setProjectDirectory(const QString& path);

signals:
    void startRequested();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void chooseProject();
    void chooseWorkDirectory();
    void updateDefaultWorkDirectory();

    QLineEdit* projectEdit_{nullptr};
    QLineEdit* workEdit_{nullptr};
    QLineEdit* cmakeEdit_{nullptr};
    QComboBox* generatorCombo_{nullptr};
    QComboBox* configurationCombo_{nullptr};
    QPlainTextEdit* argumentsEdit_{nullptr};
    QSpinBox* parallelSpin_{nullptr};
    QSpinBox* probesSpin_{nullptr};
    QSpinBox* timeoutSpin_{nullptr};
};

}  // namespace doctor::ui
