#include "doctor/ui/main_window.h"

#include "doctor/ui/analysis_controller.h"
#include "doctor/ui/analysis_workspace_widget.h"
#include "doctor/ui/project_setup_widget.h"
#include "doctor/ui/ui_theme.h"

#include <QCloseEvent>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>

namespace doctor::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      pages_(new QStackedWidget(this)),
      setup_(new ProjectSetupWidget(pages_)),
      workspace_(new AnalysisWorkspaceWidget(pages_)),
      controller_(new AnalysisController(this)) {
    setWindowTitle(tr("Unity Build Doctor"));
    setMinimumSize(1080, 720);
    resize(1320, 840);
    pages_->setObjectName(QStringLiteral("mainPages"));
    pages_->addWidget(setup_);
    pages_->addWidget(workspace_);
    setCentralWidget(pages_);
    applyTheme();

    connect(setup_, &ProjectSetupWidget::startRequested,
            this, &MainWindow::startAnalysis);
    connect(workspace_, &AnalysisWorkspaceWidget::cancelRequested,
            controller_, &AnalysisController::cancel);
    connect(workspace_, &AnalysisWorkspaceWidget::backRequested, this, [this] {
        if (controller_->isRunning()) {
            controller_->cancel();
            return;
        }
        pages_->setCurrentWidget(setup_);
    });
    connect(workspace_, &AnalysisWorkspaceWidget::exportRequested,
            this, &MainWindow::exportReport);
    connect(controller_, &AnalysisController::progressChanged,
            workspace_, &AnalysisWorkspaceWidget::updateProgress);
    connect(controller_, &AnalysisController::logReceived,
            workspace_, &AnalysisWorkspaceWidget::appendLog);
    connect(controller_, &AnalysisController::targetReceived,
            workspace_, &AnalysisWorkspaceWidget::addTarget);
    connect(controller_, &AnalysisController::analysisFinished,
            workspace_, &AnalysisWorkspaceWidget::setFinished);
}

void MainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::PaletteChange ||
        event->type() == QEvent::ApplicationPaletteChange) {
        applyTheme();
    }
}

void MainWindow::applyTheme() {
    setStyleSheet(buildUiStyleSheet(palette()));
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (controller_->isRunning()) {
        controller_->cancel();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::startAnalysis() {
    QString error;
    if (!setup_->validate(&error)) {
        QMessageBox::warning(this, tr("无法开始分析"), error);
        return;
    }
    const auto config = setup_->config();
    QSettings settings;
    settings.setValue(
        QStringLiteral("recent/project"),
        QString::fromStdString(config.sourceDirectory));
    settings.setValue(
        QStringLiteral("tools/cmake"),
        QString::fromStdString(config.cmakeExecutable));
    workspace_->reset(QFileInfo(
        QString::fromStdString(config.sourceDirectory)).fileName());
    pages_->setCurrentWidget(workspace_);
    controller_->start(config);
}

void MainWindow::exportReport() {
    const auto directory = QFileDialog::getExistingDirectory(
        this, tr("选择报告导出目录"));
    if (directory.isEmpty()) {
        return;
    }
    QString error;
    if (!controller_->exportReport(directory, &error)) {
        QMessageBox::critical(this, tr("导出失败"), error);
        return;
    }
    QMessageBox::information(
        this,
        tr("导出完成"),
        tr("已生成 JSON、Markdown 和 CMake 报告。"));
}

}  // namespace doctor::ui
