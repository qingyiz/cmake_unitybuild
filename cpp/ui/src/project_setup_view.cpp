#include "doctor/ui/project_setup_widget.h"

#include <QAbstractSpinBox>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QThread>
#include <QVBoxLayout>

namespace doctor::ui {

ProjectSetupWidget::ProjectSetupWidget(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("setupPage"));
    setAcceptDrops(true);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* canvas = new QWidget(scroll);
    canvas->setObjectName(QStringLiteral("setupPage"));
    auto* canvasLayout = new QHBoxLayout(canvas);
    canvasLayout->setContentsMargins(36, 30, 36, 34);

    auto* content = new QWidget(canvas);
    content->setObjectName(QStringLiteral("setupContent"));
    content->setMaximumWidth(1220);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(20);

    auto* eyebrow = new QLabel(tr("UNITY BUILD DOCTOR"), content);
    eyebrow->setProperty("role", "eyebrow");
    auto* heading = new QHBoxLayout;
    heading->setSpacing(16);
    auto* title = new QLabel(tr("让 Unity Build 加速，不再靠猜"), content);
    title->setProperty("role", "title");
    auto* safety = new QLabel(tr("只读分析 · 不修改源码"), content);
    safety->setProperty("role", "pill");
    heading->addWidget(title);
    heading->addStretch();
    heading->addWidget(safety, 0, Qt::AlignVCenter);
    auto* subtitle = new QLabel(
        tr("可以逐 target 对比普通构建与 Unity Build，也可以不构建项目，"
           "直接扫描源码中的常见 Unity Build 风险。"),
        content);
    subtitle->setProperty("role", "muted");
    subtitle->setWordWrap(true);
    contentLayout->addWidget(eyebrow);
    contentLayout->addLayout(heading);
    contentLayout->addWidget(subtitle);

    auto* cards = new QHBoxLayout;
    cards->setSpacing(18);

    auto* projectCard = new QFrame(content);
    projectCard->setObjectName(QStringLiteral("projectToolchainCard"));
    projectCard->setProperty("card", true);
    auto* projectCardLayout = new QVBoxLayout(projectCard);
    projectCardLayout->setContentsMargins(24, 22, 24, 24);
    projectCardLayout->setSpacing(8);
    auto* projectTitle = new QLabel(tr("01  项目与工具链"), projectCard);
    projectTitle->setProperty("role", "cardTitle");
    auto* projectHint = new QLabel(
        tr("选择源码目录；仅构建验证模式需要配置 CMake 环境。"), projectCard);
    projectHint->setProperty("role", "muted");
    projectHint->setWordWrap(true);
    projectCardLayout->addWidget(projectTitle);
    projectCardLayout->addWidget(projectHint);
    projectCardLayout->addSpacing(8);

    auto* projectRow = new QWidget(projectCard);
    auto* projectLayout = new QHBoxLayout(projectRow);
    projectLayout->setContentsMargins(0, 0, 0, 0);
    projectLayout->setSpacing(8);
    projectEdit_ = new QLineEdit(projectRow);
    projectEdit_->setObjectName(QStringLiteral("projectDirectoryEdit"));
    projectEdit_->setPlaceholderText(tr("拖入项目文件夹，或点击浏览…"));
    projectEdit_->setAccessibleName(tr("项目目录"));
    auto* projectButton = new QPushButton(tr("浏览"), projectRow);
    projectButton->setAccessibleName(tr("浏览项目目录"));
    projectLayout->addWidget(projectEdit_, 1);
    projectLayout->addWidget(projectButton);

    auto* workRow = new QWidget(projectCard);
    auto* workLayout = new QHBoxLayout(workRow);
    workLayout->setContentsMargins(0, 0, 0, 0);
    workLayout->setSpacing(8);
    workEdit_ = new QLineEdit(workRow);
    workEdit_->setObjectName(QStringLiteral("workDirectoryEdit"));
    workEdit_->setAccessibleName(tr("诊断工作目录"));
    auto* workButton = new QPushButton(tr("选择"), workRow);
    workButton->setAccessibleName(tr("选择诊断工作目录"));
    workLayout->addWidget(workEdit_, 1);
    workLayout->addWidget(workButton);

    auto* form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(11);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->addRow(tr("项目目录"), projectRow);
    form->addRow(tr("工作目录"), workRow);
    cmakeEdit_ = new QLineEdit(
        QStandardPaths::findExecutable(QStringLiteral("cmake")), projectCard);
    cmakeEdit_->setObjectName(QStringLiteral("cmakeExecutableEdit"));
    cmakeEdit_->setAccessibleName(tr("CMake 可执行文件"));
    form->addRow(tr("CMake"), cmakeEdit_);
    generatorCombo_ = new QComboBox(projectCard);
    generatorCombo_->setObjectName(QStringLiteral("generatorCombo"));
    generatorCombo_->addItems({QStringLiteral("Ninja"), QStringLiteral("Unix Makefiles")});
    form->addRow(tr("生成器"), generatorCombo_);
    configurationCombo_ = new QComboBox(projectCard);
    configurationCombo_->setObjectName(QStringLiteral("configurationCombo"));
    configurationCombo_->addItems({QStringLiteral("Debug"), QStringLiteral("Release"),
                                    QStringLiteral("RelWithDebInfo")});
    form->addRow(tr("配置"), configurationCombo_);
    projectCardLayout->addLayout(form);

    auto* optionsCard = new QFrame(content);
    optionsCard->setObjectName(QStringLiteral("analysisOptionsCard"));
    optionsCard->setProperty("card", true);
    auto* optionsCardLayout = new QVBoxLayout(optionsCard);
    optionsCardLayout->setContentsMargins(24, 22, 24, 24);
    optionsCardLayout->setSpacing(8);
    auto* optionsTitle = new QLabel(tr("02  分析策略"), optionsCard);
    optionsTitle->setProperty("role", "cardTitle");
    auto* optionsHint = new QLabel(
        tr("选择分析方式；构建参数只在构建验证模式中生效。"), optionsCard);
    optionsHint->setProperty("role", "muted");
    optionsHint->setWordWrap(true);
    optionsCardLayout->addWidget(optionsTitle);
    optionsCardLayout->addWidget(optionsHint);
    optionsCardLayout->addSpacing(8);

    auto* options = new QFormLayout;
    options->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    options->setContentsMargins(0, 0, 0, 0);
    options->setHorizontalSpacing(12);
    options->setVerticalSpacing(11);
    options->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    analysisModeCombo_ = new QComboBox(optionsCard);
    analysisModeCombo_->setObjectName(QStringLiteral("analysisModeCombo"));
    analysisModeCombo_->addItem(
        tr("构建验证（CMake）"),
        static_cast<int>(doctor::domain::AnalysisMode::BuildVerification));
    analysisModeCombo_->addItem(
        tr("源码快速扫描（无需构建）"),
        static_cast<int>(doctor::domain::AnalysisMode::SourceScan));
    parallelSpin_ = new QSpinBox(optionsCard);
    parallelSpin_->setObjectName(QStringLiteral("parallelJobsSpin"));
    parallelSpin_->setRange(0, 128);
    parallelSpin_->setValue(qMax(1, QThread::idealThreadCount()));
    parallelSpin_->setSpecialValueText(tr("自动"));
    parallelSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    probesSpin_ = new QSpinBox(optionsCard);
    probesSpin_->setObjectName(QStringLiteral("maximumProbesSpin"));
    probesSpin_->setRange(1, 10000);
    probesSpin_->setValue(100);
    probesSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    timeoutSpin_ = new QSpinBox(optionsCard);
    timeoutSpin_->setObjectName(QStringLiteral("timeoutSpin"));
    timeoutSpin_->setRange(10, 86400);
    timeoutSpin_->setValue(300);
    timeoutSpin_->setSuffix(tr(" 秒"));
    timeoutSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    argumentsEdit_ = new QPlainTextEdit(optionsCard);
    argumentsEdit_->setObjectName(QStringLiteral("cmakeArgumentsEdit"));
    argumentsEdit_->setPlaceholderText(
        tr("每行一个 CMake 参数，例如：\n-DCMAKE_PREFIX_PATH=/path/to/Qt"));
    argumentsEdit_->setMinimumHeight(104);
    argumentsEdit_->setMaximumHeight(128);
    options->addRow(tr("分析模式"), analysisModeCombo_);
    options->addRow(tr("并行任务"), parallelSpin_);
    options->addRow(tr("最大探针"), probesSpin_);
    options->addRow(tr("超时"), timeoutSpin_);
    options->addRow(tr("附加参数"), argumentsEdit_);
    optionsCardLayout->addLayout(options);
    optionsCardLayout->addStretch();

    cards->addWidget(projectCard, 3);
    cards->addWidget(optionsCard, 2);
    contentLayout->addLayout(cards);

    auto* actionCard = new QFrame(content);
    actionCard->setProperty("softCard", true);
    auto* actionLayout = new QHBoxLayout(actionCard);
    actionLayout->setContentsMargins(20, 14, 16, 14);
    actionLayout->setSpacing(16);
    auto* actionText = new QVBoxLayout;
    actionText->setSpacing(2);
    actionTitle_ = new QLabel(tr("准备好后开始完整诊断"), actionCard);
    actionTitle_->setProperty("role", "cardTitle");
    actionHint_ = new QLabel(
        tr("构建与探针文件只会写入诊断工作目录。"), actionCard);
    actionHint_->setProperty("role", "muted");
    actionText->addWidget(actionTitle_);
    actionText->addWidget(actionHint_);

    startButton_ = new QPushButton(tr("开始分析整个项目  →"), actionCard);
    startButton_->setObjectName(QStringLiteral("startAnalysisButton"));
    startButton_->setProperty("role", "primary");
    startButton_->setAccessibleName(tr("开始分析整个项目"));
    startButton_->setMinimumWidth(220);
    startButton_->setDefault(true);
    actionLayout->addLayout(actionText, 1);
    actionLayout->addWidget(startButton_, 0, Qt::AlignVCenter);
    contentLayout->addWidget(actionCard);
    contentLayout->addStretch();

    canvasLayout->addStretch();
    canvasLayout->addWidget(content, 1);
    canvasLayout->addStretch();
    scroll->setWidget(canvas);
    root->addWidget(scroll);

    connect(projectButton, &QPushButton::clicked, this, &ProjectSetupWidget::chooseProject);
    connect(workButton, &QPushButton::clicked, this, &ProjectSetupWidget::chooseWorkDirectory);
    connect(projectEdit_, &QLineEdit::editingFinished,
            this, &ProjectSetupWidget::updateDefaultWorkDirectory);
    connect(
        analysisModeCombo_,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        [this] { updateModeUi(); });
    connect(
        startButton_, &QPushButton::clicked,
        this, &ProjectSetupWidget::startRequested);

    QSettings settings;
    setProjectDirectory(settings.value(QStringLiteral("recent/project")).toString());
    cmakeEdit_->setText(settings.value(
        QStringLiteral("tools/cmake"), cmakeEdit_->text()).toString());
    const auto savedMode = settings.value(
        QStringLiteral("analysis/mode"),
        static_cast<int>(doctor::domain::AnalysisMode::BuildVerification)).toInt();
    const auto modeIndex = analysisModeCombo_->findData(savedMode);
    analysisModeCombo_->setCurrentIndex(modeIndex < 0 ? 0 : modeIndex);
    updateModeUi();
}

}  // namespace doctor::ui
