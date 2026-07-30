#include "doctor/ui/analysis_workspace_widget.h"

#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTextCursor>
#include <QToolButton>
#include <QVBoxLayout>

namespace doctor::ui {

AnalysisWorkspaceWidget::AnalysisWorkspaceWidget(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("workspacePage"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(14);

    auto* headerCard = new QFrame(this);
    headerCard->setProperty("card", true);
    auto* top = new QHBoxLayout(headerCard);
    top->setContentsMargins(18, 14, 18, 14);
    top->setSpacing(10);
    auto* back = new QPushButton(tr("←  项目设置"), headerCard);
    back->setProperty("role", "quiet");
    auto* projectText = new QVBoxLayout;
    projectText->setSpacing(0);
    auto* workspaceEyebrow = new QLabel(tr("ANALYSIS WORKSPACE"), headerCard);
    workspaceEyebrow->setProperty("role", "eyebrow");
    projectLabel_ = new QLabel(headerCard);
    projectLabel_->setProperty("role", "pageTitle");
    projectText->addWidget(workspaceEyebrow);
    projectText->addWidget(projectLabel_);
    statusLabel_ = new QLabel(tr("准备中"), headerCard);
    statusLabel_->setProperty("role", "pill");
    cancelButton_ = new QPushButton(tr("取消分析"), headerCard);
    cancelButton_->setObjectName(QStringLiteral("cancelAnalysisButton"));
    cancelButton_->setProperty("role", "danger");
    exportButton_ = new QPushButton(tr("导出报告"), headerCard);
    exportButton_->setObjectName(QStringLiteral("exportReportButton"));
    exportButton_->setProperty("role", "primary");
    exportButton_->setEnabled(false);
    top->addWidget(back);
    top->addSpacing(4);
    top->addLayout(projectText);
    top->addStretch();
    top->addWidget(statusLabel_);
    top->addWidget(cancelButton_);
    top->addWidget(exportButton_);
    root->addWidget(headerCard);

    auto* progressCard = new QFrame(this);
    progressCard->setProperty("softCard", true);
    auto* progressLayout = new QVBoxLayout(progressCard);
    progressLayout->setContentsMargins(16, 12, 16, 12);
    progressLayout->setSpacing(8);
    auto* progressRow = new QHBoxLayout;
    progressTitle_ = new QLabel(tr("项目分析进度"), progressCard);
    progressTitle_->setProperty("role", "cardTitle");
    summaryLabel_ = new QLabel(tr("正在发现 target…"), progressCard);
    summaryLabel_->setProperty("role", "muted");
    progressRow->addWidget(progressTitle_);
    progressRow->addStretch();
    progressRow->addWidget(summaryLabel_);
    progress_ = new QProgressBar(progressCard);
    progress_->setRange(0, 0);
    progress_->setTextVisible(false);
    progressLayout->addLayout(progressRow);
    progressLayout->addWidget(progress_);
    root->addWidget(progressCard);

    horizontalSplitter_ = new QSplitter(Qt::Horizontal, this);
    horizontalSplitter_->setObjectName(QStringLiteral("workspaceSplitter"));
    horizontalSplitter_->setChildrenCollapsible(false);
    horizontalSplitter_->setHandleWidth(10);

    targetsPane_ = new QFrame(horizontalSplitter_);
    targetsPane_->setObjectName(QStringLiteral("targetsPane"));
    targetsPane_->setProperty("card", true);
    auto* targetsLayout = new QVBoxLayout(targetsPane_);
    targetsLayout->setContentsMargins(14, 14, 14, 14);
    targetsLayout->setSpacing(10);
    auto* targetsHeader = new QHBoxLayout;
    resultsTitle_ = new QLabel(tr("Target 结果"), targetsPane_);
    resultsTitle_->setProperty("role", "cardTitle");
    filterEdit_ = new QLineEdit(targetsPane_);
    filterEdit_->setObjectName(QStringLiteral("resultFilterEdit"));
    filterEdit_->setPlaceholderText(tr("搜索 target、状态或分类…"));
    filterEdit_->setAccessibleName(tr("结果筛选"));
    filterEdit_->setMaximumWidth(320);
    targetsHeader->addWidget(resultsTitle_);
    targetsHeader->addStretch();
    targetsHeader->addWidget(filterEdit_);
    targetsLayout->addLayout(targetsHeader);

    targetsTable_ = new QTableWidget(0, 4, targetsPane_);
    targetsTable_->setObjectName(QStringLiteral("targetsTable"));
    targetsTable_->setHorizontalHeaderLabels(
        {tr("Target"), tr("类型"), tr("状态"), tr("问题")});
    targetsTable_->horizontalHeader()->setStretchLastSection(true);
    targetsTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    targetsTable_->horizontalHeader()->setHighlightSections(false);
    targetsTable_->verticalHeader()->setVisible(false);
    targetsTable_->verticalHeader()->setDefaultSectionSize(38);
    targetsTable_->setAlternatingRowColors(true);
    targetsTable_->setShowGrid(false);
    targetsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    targetsTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    targetsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    targetsLayout->addWidget(targetsTable_);

    auto* detailPane = new QFrame(horizontalSplitter_);
    detailPane->setProperty("card", true);
    auto* detailLayout = new QVBoxLayout(detailPane);
    detailLayout->setContentsMargins(14, 14, 14, 14);
    detailLayout->setSpacing(10);
    auto* detailHeader = new QHBoxLayout;
    auto* detailTitle = new QLabel(tr("问题详情"), detailPane);
    detailTitle->setProperty("role", "cardTitle");
    focusDetailsButton_ = new QPushButton(tr("专注详情"), detailPane);
    focusDetailsButton_->setObjectName(QStringLiteral("focusDetailsButton"));
    focusDetailsButton_->setProperty("role", "quiet");
    focusDetailsButton_->setAccessibleName(tr("展开问题详情区域"));
    auto* copy = new QPushButton(tr("复制 CMake 建议"), detailPane);
    detailHeader->addWidget(detailTitle);
    detailHeader->addStretch();
    detailHeader->addWidget(focusDetailsButton_);
    detailHeader->addWidget(copy);
    detailLayout->addLayout(detailHeader);
    detailSplitter_ = new QSplitter(Qt::Vertical, detailPane);
    detailSplitter_->setObjectName(QStringLiteral("detailContentSplitter"));
    detailSplitter_->setChildrenCollapsible(false);
    detailSplitter_->setHandleWidth(10);
    details_ = new QTextBrowser(detailSplitter_);
    details_->setObjectName(QStringLiteral("issueDetails"));
    details_->setOpenExternalLinks(false);
    details_->setMinimumHeight(180);
    cmake_ = new QPlainTextEdit(detailSplitter_);
    cmake_->setObjectName(QStringLiteral("cmakeSuggestion"));
    cmake_->setReadOnly(true);
    cmake_->setPlaceholderText(tr("选择包含问题的 target 后显示 CMake 建议"));
    cmake_->setMinimumHeight(100);
    detailSplitter_->addWidget(details_);
    detailSplitter_->addWidget(cmake_);
    detailSplitter_->setStretchFactor(0, 4);
    detailSplitter_->setStretchFactor(1, 1);
    detailSplitter_->setSizes({420, 140});
    detailLayout->addWidget(detailSplitter_, 1);
    horizontalSplitter_->addWidget(targetsPane_);
    horizontalSplitter_->addWidget(detailPane);
    horizontalSplitter_->setStretchFactor(0, 2);
    horizontalSplitter_->setStretchFactor(1, 3);
    horizontalSplitter_->setSizes({460, 660});
    root->addWidget(horizontalSplitter_, 1);

    logCard_ = new QFrame(this);
    logCard_->setObjectName(QStringLiteral("logCard"));
    logCard_->setProperty("card", true);
    auto* logLayout = new QVBoxLayout(logCard_);
    logLayout->setContentsMargins(14, 12, 14, 14);
    logLayout->setSpacing(8);
    auto* logHeader = new QHBoxLayout;
    auto* logTitle = new QLabel(tr("实时日志"), logCard_);
    logTitle->setProperty("role", "cardTitle");
    auto* logHint = new QLabel(
        tr("显示最近 10,000 行 · 完整日志保存在工作目录"), logCard_);
    logHint->setProperty("role", "muted");
    logHeader->addWidget(logTitle);
    logHeader->addStretch();
    logHeader->addWidget(logHint);
    logs_ = new QPlainTextEdit(logCard_);
    logs_->setObjectName(QStringLiteral("analysisLogs"));
    logs_->setReadOnly(true);
    logs_->document()->setMaximumBlockCount(10000);
    logs_->setMaximumHeight(140);
    logLayout->addLayout(logHeader);
    logLayout->addWidget(logs_);
    root->addWidget(logCard_);

    connect(back, &QPushButton::clicked, this, &AnalysisWorkspaceWidget::backRequested);
    connect(cancelButton_, &QPushButton::clicked,
            this, &AnalysisWorkspaceWidget::cancelRequested);
    connect(exportButton_, &QPushButton::clicked,
            this, &AnalysisWorkspaceWidget::exportRequested);
    connect(filterEdit_, &QLineEdit::textChanged,
            this, &AnalysisWorkspaceWidget::applyFilter);
    connect(targetsTable_, &QTableWidget::cellClicked,
            this, [this](int row, int) { showTargetDetails(row); });
    connect(focusDetailsButton_, &QPushButton::clicked, this, [this] {
        setDetailFocus(!detailFocused_);
    });
    connect(copy, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(cmake_->toPlainText());
    });
}

}  // namespace doctor::ui
