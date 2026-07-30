#include "doctor/ui/analysis_workspace_widget.h"

#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTextCursor>

namespace doctor::ui {

void AnalysisWorkspaceWidget::reset(const QString& project, bool sourceScan) {
    setDetailFocus(false);
    detailSplitter_->setSizes({420, 140});
    sourceScan_ = sourceScan;
    projectLabel_->setText(project);
    statusLabel_->setText(tr("正在分析"));
    progress_->setRange(0, 0);
    progressTitle_->setText(sourceScan_ ? tr("源码扫描进度") : tr("项目分析进度"));
    resultsTitle_->setText(sourceScan_ ? tr("规则检查结果") : tr("Target 结果"));
    targetsTable_->setHorizontalHeaderLabels(
        sourceScan_
            ? QStringList{tr("检查项"), tr("类型"), tr("状态"), tr("风险")}
            : QStringList{tr("Target"), tr("类型"), tr("状态"), tr("问题")});
    summaryLabel_->setText(
        sourceScan_ ? tr("正在读取源码并匹配规则…") : tr("正在发现 target…"));
    cancelButton_->setEnabled(true);
    exportButton_->setEnabled(false);
    targetsTable_->setRowCount(0);
    details_->setHtml(
        sourceScan_
            ? tr("<h3>等待扫描结果</h3><p>选择检查项查看风险详情。</p>")
            : tr("<h3>等待分析结果</h3><p>选择 target 查看问题详情。</p>"));
    cmake_->clear();
    logs_->clear();
}

void AnalysisWorkspaceWidget::setDetailFocus(bool focused) {
    detailFocused_ = focused;
    targetsPane_->setVisible(!focused);
    logCard_->setVisible(!focused);
    focusDetailsButton_->setText(
        focused ? tr("退出专注") : tr("专注详情"));
    focusDetailsButton_->setAccessibleName(
        focused ? tr("恢复结果列表和实时日志") : tr("展开问题详情区域"));
    if (!focused) {
        horizontalSplitter_->setSizes({460, 660});
    }
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
    int riskChecks = 0;
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
        riskChecks += status == QStringLiteral("Risk Found") ? 1 : 0;
        issues += issueCount;
    }
    summaryLabel_->setText(sourceScan_
        ? tr("共 %1 个检查项 · 通过 %2 · 存在风险 %3 · 风险条目 %4")
            .arg(targetsTable_->rowCount())
            .arg(passed)
            .arg(riskChecks)
            .arg(issues)
        : tr("共 %1 个 target · 通过 %2 · 普通失败 %3 · Unity 问题 %4 · 问题 %5")
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
        html += sourceScan_
            ? tr("<p>静态扫描未发现这一类常见 Unity Build 风险。</p>")
            : tr("<p>没有发现 Unity Build 问题。</p>");
    } else {
        QStringList snippets;
        for (const auto& issueValue : issues) {
            const auto issue = issueValue.toMap();
            QStringList escapedSources;
            for (const auto& source : issue.value("sources").toStringList()) {
                escapedSources.push_back(source.toHtmlEscaped());
            }
            QStringList escapedEvidence;
            for (const auto& evidence : issue.value("evidence").toStringList()) {
                escapedEvidence.push_back(evidence.toHtmlEscaped());
            }
            const auto ruleId = issue.value("ruleId").toString().toHtmlEscaped();
            const auto ruleMetadata = ruleId.isEmpty()
                ? QString()
                : QStringLiteral("<b>规则：</b><code>%1</code> · ").arg(ruleId);
            html += QStringLiteral(
                "<hr><h3>%1</h3><p>%2</p>"
                "<p>%3<b>级别：</b>%4 · "
                "<b>置信度：</b>%5%</p>"
                "<p><b>指纹：</b><code>%6</code></p>"
                "<p><b>涉及文件：</b><br>%7</p>"
                "<p><b>证据：</b><br>%8</p><p><b>建议：</b>%9</p>")
                .arg(issue.value("category").toString().toHtmlEscaped(),
                     issue.value("summary").toString().toHtmlEscaped(),
                     ruleMetadata,
                     issue.value("severity").toString().toHtmlEscaped(),
                     QString::number(
                         issue.value("confidence").toDouble() * 100.0, 'f', 0),
                     issue.value("fingerprint").toString().toHtmlEscaped(),
                     escapedSources.join(QStringLiteral("<br>")),
                     escapedEvidence.join(QStringLiteral("<br>")),
                     issue.value("suggestion").toString().toHtmlEscaped());
            if (!issue.value("cmake").toString().isEmpty()) {
                snippets.push_back(issue.value("cmake").toString());
            }
        }
        cmake_->setPlainText(snippets.join(QStringLiteral("\n\n")));
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
