#include "doctor/ui/project_setup_widget.h"

#include <QComboBox>
#include <QCryptographicHash>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QStandardPaths>

namespace doctor::ui {

doctor::domain::ProjectConfig ProjectSetupWidget::config() const {
    doctor::domain::ProjectConfig value;
    value.sourceDirectory = projectEdit_->text().toStdString();
    value.workDirectory = workEdit_->text().toStdString();
    value.cmakeExecutable = cmakeEdit_->text().toStdString();
    value.generator = generatorCombo_->currentText().toStdString();
    value.configuration = configurationCombo_->currentText().toStdString();
    value.parallelJobs = parallelSpin_->value();
    value.maxProbes = probesSpin_->value();
    value.timeoutSeconds = timeoutSpin_->value();
    for (const auto& line : argumentsEdit_->toPlainText().split('\n')) {
        if (!line.trimmed().isEmpty()) {
            value.cmakeArguments.push_back(line.trimmed().toStdString());
        }
    }
    return value;
}

bool ProjectSetupWidget::validate(QString* error) const {
    const QDir source(projectEdit_->text());
    if (!source.exists() || !QFileInfo(source.filePath("CMakeLists.txt")).isFile()) {
        *error = tr("请选择包含 CMakeLists.txt 的项目目录。");
        return false;
    }
    if (!QFileInfo(cmakeEdit_->text()).isExecutable()) {
        *error = tr("CMake 可执行文件不存在或不可执行。");
        return false;
    }
    const auto sourcePath = QFileInfo(source.absolutePath()).canonicalFilePath();
    const auto workPath = QFileInfo(workEdit_->text()).absoluteFilePath();
    if (workPath == sourcePath ||
        workPath.startsWith(sourcePath + QDir::separator()) ||
        sourcePath.startsWith(workPath + QDir::separator())) {
        *error = tr("工作目录必须位于项目目录之外。");
        return false;
    }
    return true;
}

void ProjectSetupWidget::setProjectDirectory(const QString& path) {
    if (path.isEmpty()) {
        return;
    }
    projectEdit_->setText(QDir(path).absolutePath());
    updateDefaultWorkDirectory();
}

void ProjectSetupWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void ProjectSetupWidget::dropEvent(QDropEvent* event) {
    const auto urls = event->mimeData()->urls();
    if (!urls.isEmpty() && QFileInfo(urls.first().toLocalFile()).isDir()) {
        setProjectDirectory(urls.first().toLocalFile());
        event->acceptProposedAction();
    }
}

void ProjectSetupWidget::chooseProject() {
    const auto path = QFileDialog::getExistingDirectory(
        this, tr("选择 CMake 项目"), projectEdit_->text());
    if (!path.isEmpty()) {
        setProjectDirectory(path);
    }
}

void ProjectSetupWidget::chooseWorkDirectory() {
    const auto path = QFileDialog::getExistingDirectory(
        this, tr("选择诊断工作目录"), workEdit_->text());
    if (!path.isEmpty()) {
        workEdit_->setText(path);
    }
}

void ProjectSetupWidget::updateDefaultWorkDirectory() {
    if (projectEdit_->text().isEmpty()) {
        return;
    }
    const auto hash = QCryptographicHash::hash(
        projectEdit_->text().toUtf8(), QCryptographicHash::Sha256).toHex().left(12);
    const auto root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    workEdit_->setText(QDir(root).filePath(
        QStringLiteral("projects/%1").arg(QString::fromLatin1(hash))));
}

}  // namespace doctor::ui
