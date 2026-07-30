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
    auto* progressTitle = new QLabel(tr("项目分析进度"), progressCard);
    progressTitle->setProperty("role", "cardTitle");
    summaryLabel_ = new QLabel(tr("正在发现 target…"), progressCard);
    summaryLabel_->setProperty("role", "muted");
    progressRow->addWidget(progressTitle);
    progressRow->addStretch();
    progressRow->addWidget(summaryLabel_);
    progress_ = new QProgressBar(progressCard);
    progress_->setRange(0, 0);
    progress_->setTextVisible(false);
    progressLayout->addLayout(progressRow);
    progressLayout->addWidget(progress_);
    root->addWidget(progressCard);

    auto* horizontal = new QSplitter(Qt::Horizontal, this);
    horizontal->setChildrenCollapsible(false);

    auto* targetsPane = new QFrame(horizontal);
    targetsPane->setProperty("card", true);
    auto* targetsLayout = new QVBoxLayout(targetsPane);
    targetsLayout->setContentsMargins(14, 14, 14, 14);
    targetsLayout->setSpacing(10);
    auto* targetsHeader = new QHBoxLayout;
    auto* targetsTitle = new QLabel(tr("Target 结果"), targetsPane);
    targetsTitle->setProperty("role", "cardTitle");
    filterEdit_ = new QLineEdit(targetsPane);
    filterEdit_->setObjectName(QStringLiteral("resultFilterEdit"));
    filterEdit_->setPlaceholderText(tr("搜索 target、状态或分类…"));
    filterEdit_->setAccessibleName(tr("结果筛选"));
    filterEdit_->setMaximumWidth(320);
    targetsHeader->addWidget(targetsTitle);
    targetsHeader->addStretch();
    targetsHeader->addWidget(filterEdit_);
    targetsLayout->addLayout(targetsHeader);

    targetsTable_ = new QTableWidget(0, 4, targetsPane);
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

    auto* detailPane = new QFrame(horizontal);
    detailPane->setProperty("card", true);
    auto* detailLayout = new QVBoxLayout(detailPane);
    detailLayout->setContentsMargins(14, 14, 14, 14);
    detailLayout->setSpacing(10);
    auto* detailHeader = new QHBoxLayout;
    auto* detailTitle = new QLabel(tr("问题详情"), detailPane);
    detailTitle->setProperty("role", "cardTitle");
    auto* copy = new QPushButton(tr("复制 CMake 建议"), detailPane);
    detailHeader->addWidget(detailTitle);
    detailHeader->addStretch();
    detailHeader->addWidget(copy);
    detailLayout->addLayout(detailHeader);
    details_ = new QTextBrowser(detailPane);
    details_->setObjectName(QStringLiteral("issueDetails"));
    details_->setOpenExternalLinks(false);
    cmake_ = new QPlainTextEdit(detailPane);
    cmake_->setObjectName(QStringLiteral("cmakeSuggestion"));
    cmake_->setReadOnly(true);
    cmake_->setPlaceholderText(tr("选择包含问题的 target 后显示 CMake 建议"));
    cmake_->setMaximumHeight(150);
    detailLayout->addWidget(details_, 1);
    detailLayout->addWidget(cmake_);
    horizontal->addWidget(targetsPane);
    horizontal->addWidget(detailPane);
    horizontal->setSizes({610, 510});
    root->addWidget(horizontal, 1);

    auto* logCard = new QFrame(this);
    logCard->setProperty("card", true);
    auto* logLayout = new QVBoxLayout(logCard);
    logLayout->setContentsMargins(14, 12, 14, 14);
    logLayout->setSpacing(8);
    auto* logHeader = new QHBoxLayout;
    auto* logTitle = new QLabel(tr("实时日志"), logCard);
    logTitle->setProperty("role", "cardTitle");
    auto* logHint = new QLabel(tr("显示最近 10,000 行 · 完整日志保存在工作目录"), logCard);
    logHint->setProperty("role", "muted");
    logHeader->addWidget(logTitle);
    logHeader->addStretch();
    logHeader->addWidget(logHint);
    logs_ = new QPlainTextEdit(logCard);
    logs_->setObjectName(QStringLiteral("analysisLogs"));
    logs_->setReadOnly(true);
    logs_->document()->setMaximumBlockCount(10000);
    logs_->setMaximumHeight(140);
    logLayout->addLayout(logHeader);
    logLayout->addWidget(logs_);
    root->addWidget(logCard);

    connect(back, &QPushButton::clicked, this, &AnalysisWorkspaceWidget::backRequested);
    connect(cancelButton_, &QPushButton::clicked,
            this, &AnalysisWorkspaceWidget::cancelRequested);
    connect(exportButton_, &QPushButton::clicked,
            this, &AnalysisWorkspaceWidget::exportRequested);
    connect(filterEdit_, &QLineEdit::textChanged,
            this, &AnalysisWorkspaceWidget::applyFilter);
    connect(targetsTable_, &QTableWidget::cellClicked,
            this, [this](int row, int) { showTargetDetails(row); });
    connect(copy, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(cmake_->toPlainText());
    });
}

}  // namespace doctor::ui
