#pragma once

#include <QMainWindow>

class QLabel;
class QPushButton;
class QStackedWidget;
class QCloseEvent;
class QEvent;

namespace doctor::ui {

class AnalysisController;
class AnalysisWorkspaceWidget;
class ProjectSetupWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void applyTheme();
    void startAnalysis();
    void exportReport();

    QStackedWidget* pages_{nullptr};
    ProjectSetupWidget* setup_{nullptr};
    AnalysisWorkspaceWidget* workspace_{nullptr};
    AnalysisController* controller_{nullptr};
};

}  // namespace doctor::ui
