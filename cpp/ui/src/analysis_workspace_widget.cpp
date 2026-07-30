#include "doctor/ui/analysis_workspace_widget.h"

#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTextCursor>

namespace doctor::ui {

void AnalysisWorkspaceWidget::reset(const QString& project) {
    projectLabel_->setText(project);
    statusLabel_->setText(tr("正在分析"));
    progress_->setRange(0, 0);
    summaryLabel_->setText(tr("正在发现 target…"));
    cancelButton_->setEnabled(true);
    exportButton_->setEnabled(false);
    targetsTable_->setRowCount(0);
    details_->setHtml(tr("<h3>等待分析结果</h3><p>选择 target 查看问题详情。</p>"));
    cmake_->clear();
    logs_->clear();
}

void AnalysisWorkspaceWidget::updateProgress(
    const QString& stage,
    const QString& target,
    const QString& message,
    int completed,
    int total) {
    statusLabel_->setText(stage);
    summaryLabel_->setText(target.isEmpty() ? message : target + QStringLiteral(" · ") + message);
    if (total > 0) {
        progress_->setRange(0, total);
        progress_->setValue(completed);
    }
}

void AnalysisWorkspaceWidget::appendLog(const QString& text) {
    logs_->moveCursor(QTextCursor::End);
    logs_->insertPlainText(text.endsWith('\n') ? text : text + '\n');
    logs_->moveCursor(QTextCursor::End);
}

void AnalysisWorkspaceWidget::addTarget(const QVariantMap& target) {
    const int row = targetsTable_->rowCount();
    targetsTable_->insertRow(row);
    const auto issues = target.value("issues").toList();
    targetsTable_->setItem(row, 0, new QTableWidgetItem(target.value("name").toString()));
    targetsTable_->setItem(row, 1, new QTableWidgetItem(target.value("type").toString()));
    targetsTable_->setItem(row, 2, new QTableWidgetItem(target.value("status").toString()));
    targetsTable_->setItem(row, 3, new QTableWidgetItem(QString::number(issues.size())));
    targetsTable_->item(row, 0)->setData(Qt::UserRole, target);
    applyFilter(filterEdit_->text());
    if (row == 0) {
        targetsTable_->selectRow(0);
        showTargetDetails(0);
    }
}

void AnalysisWorkspaceWidget::setFinished(bool cancelled) {
    statusLabel_->setText(cancelled ? tr("已取消") : tr("分析完成"));
    cancelButton_->setEnabled(false);
    exportButton_->setEnabled(true);
    int passed = 0;
    int baselineFailed = 0;
    int unityFailed = 0;
    int issues = 0;
    for (int row = 0; row < targetsTable_->rowCount(); ++row) {
        const auto status = targetsTable_->item(row, 2)->text();
        const auto issueCount = targetsTable_->item(row, 3)->text().toInt();
        passed += status == QStringLiteral("Passed") ? 1 : 0;
        baselineFailed += status == QStringLiteral("Baseline Failed") ? 1 : 0;
        unityFailed +=
            (status == QStringLiteral("Unity Failed") ||
             status == QStringLiteral("Non Replayable"))
            ? 1 : 0;
        issues += issueCount;
    }
    summaryLabel_->setText(
        tr("共 %1 个 target · 通过 %2 · 普通失败 %3 · Unity 问题 %4 · 问题 %5")
            .arg(targetsTable_->rowCount())
            .arg(passed)
            .arg(baselineFailed)
            .arg(unityFailed)
            .arg(issues));
}

void AnalysisWorkspaceWidget::showTargetDetails(int row) {
    const auto target = targetsTable_->item(row, 0)->data(Qt::UserRole).toMap();
    const auto issues = target.value("issues").toList();
    QString html = QStringLiteral("<h2>%1</h2><p><b>状态：</b>%2<br><b>阶段：</b>%3</p>")
        .arg(target.value("name").toString().toHtmlEscaped(),
             target.value("status").toString().toHtmlEscaped(),
             target.value("stage").toString().toHtmlEscaped());
    cmake_->clear();
    if (issues.isEmpty()) {
        html += tr("<p>没有发现 Unity Build 问题。</p>");
    } else {
        const auto issue = issues.first().toMap();
        QStringList escapedSources;
        for (const auto& source : issue.value("sources").toStringList()) {
            escapedSources.push_back(source.toHtmlEscaped());
        }
        QStringList escapedEvidence;
        for (const auto& evidence : issue.value("evidence").toStringList()) {
            escapedEvidence.push_back(evidence.toHtmlEscaped());
        }
        html += QStringLiteral(
            "<hr><h3>%1</h3><p>%2</p><p><b>置信度：</b>%3%</p>"
            "<p><b>失败指纹：</b><code>%4</code></p>"
            "<p><b>最小冲突文件：</b><br>%5</p>"
            "<p><b>证据：</b><br>%6</p><p><b>建议：</b>%7</p>")
            .arg(issue.value("category").toString().toHtmlEscaped(),
                 issue.value("summary").toString().toHtmlEscaped(),
                 QString::number(issue.value("confidence").toDouble() * 100.0, 'f', 0),
                 issue.value("fingerprint").toString().toHtmlEscaped(),
                 escapedSources.join(QStringLiteral("<br>")),
                 escapedEvidence.join(QStringLiteral("<br>")),
                 issue.value("suggestion").toString().toHtmlEscaped());
        cmake_->setPlainText(issue.value("cmake").toString());
    }
    details_->setHtml(html);
}

void AnalysisWorkspaceWidget::applyFilter(const QString& text) {
    for (int row = 0; row < targetsTable_->rowCount(); ++row) {
        QString combined;
        for (int column = 0; column < targetsTable_->columnCount(); ++column) {
            combined += targetsTable_->item(row, column)->text() + ' ';
        }
        const auto data = targetsTable_->item(row, 0)->data(Qt::UserRole).toMap();
        for (const auto& issue : data.value("issues").toList()) {
            combined += issue.toMap().value("category").toString() + ' ';
        }
        targetsTable_->setRowHidden(
            row, !combined.contains(text, Qt::CaseInsensitive));
    }
}

}  // namespace doctor::ui
