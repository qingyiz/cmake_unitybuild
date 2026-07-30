#include "doctor/ui/analysis_controller.h"
#include "doctor/ui/analysis_workspace_widget.h"
#include "doctor/ui/project_setup_widget.h"
#include "doctor/ui/ui_theme.h"

#include <QFile>
#include <QComboBox>
#include <QFrame>
#include <QLineEdit>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QSplitter>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QtTest>

class UiTest final : public QObject {
    Q_OBJECT

private slots:
    void setupPageKeepsHierarchyAtMinimumWindowSize() {
        doctor::ui::ProjectSetupWidget widget;
        widget.resize(1080, 720);
        widget.show();
        QTest::qWait(50);

        const auto cards = widget.findChildren<QFrame*>();
        int visualCards = 0;
        for (const auto* card : cards) {
            visualCards += card->property("card").toBool() ? 1 : 0;
        }
        auto* start = widget.findChild<QPushButton*>(
            QStringLiteral("startAnalysisButton"));
        QVERIFY(start);
        QVERIFY(start->property("role").toString() == QStringLiteral("primary"));
        QVERIFY(visualCards >= 2);
        QVERIFY(!start->visibleRegion().isEmpty());
    }

    void themeIsDerivedForLightAndDarkPalettes() {
        QPalette light;
        light.setColor(QPalette::Window, QColor(QStringLiteral("#f4f5f7")));
        light.setColor(QPalette::Base, Qt::white);
        light.setColor(QPalette::Text, QColor(QStringLiteral("#18202a")));
        light.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#68717d")));
        light.setColor(QPalette::Highlight, QColor(QStringLiteral("#2563eb")));
        light.setColor(QPalette::HighlightedText, Qt::white);
        auto dark = light;
        dark.setColor(QPalette::Window, QColor(QStringLiteral("#15181d")));
        dark.setColor(QPalette::Base, QColor(QStringLiteral("#20242b")));
        dark.setColor(QPalette::Text, QColor(QStringLiteral("#eef2f7")));
        dark.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#9aa4b2")));
        dark.setColor(QPalette::Highlight, QColor(QStringLiteral("#6ea8fe")));
        dark.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#111827")));

        const auto lightStyle = doctor::ui::buildUiStyleSheet(light);
        const auto darkStyle = doctor::ui::buildUiStyleSheet(dark);
        QVERIFY(lightStyle.contains(QStringLiteral("QFrame[card=\"true\"]")));
        QVERIFY(lightStyle.contains(QStringLiteral("QPushButton[role=\"primary\"]")));
        QVERIFY(lightStyle != darkStyle);
        QVERIFY(!darkStyle.contains(QStringLiteral("background: #ffffff")));
    }

    void validatesProjectAndRejectsOverlappingWorkTree() {
        doctor::ui::ProjectSetupWidget widget;
        widget.setProjectDirectory(QStringLiteral(TEST_FIXTURE_DIRECTORY));
        auto* cmake = widget.findChild<QLineEdit*>(
            QStringLiteral("cmakeExecutableEdit"));
        auto* work = widget.findChild<QLineEdit*>(
            QStringLiteral("workDirectoryEdit"));
        QVERIFY(cmake);
        QVERIFY(work);
        cmake->setText(QStringLiteral(TEST_CMAKE_COMMAND));
        QString error;
        QVERIFY2(widget.validate(&error), qPrintable(error));

        work->setText(QStringLiteral(TEST_FIXTURE_DIRECTORY));
        QVERIFY(!widget.validate(&error));
        QVERIFY(error.contains(QStringLiteral("之外")));
    }

    void sourceScanModeDoesNotRequireCMakeProject() {
        doctor::ui::ProjectSetupWidget widget;
        widget.setProjectDirectory(QStringLiteral(TEST_SOURCE_SCAN_DIRECTORY));
        auto* mode = widget.findChild<QComboBox*>(
            QStringLiteral("analysisModeCombo"));
        auto* cmake = widget.findChild<QLineEdit*>(
            QStringLiteral("cmakeExecutableEdit"));
        QVERIFY(mode);
        QVERIFY(cmake);
        mode->setCurrentIndex(mode->findData(
            static_cast<int>(doctor::domain::AnalysisMode::SourceScan)));
        cmake->setText(QStringLiteral("/does/not/exist"));

        QString error;
        QVERIFY2(widget.validate(&error), qPrintable(error));
        QVERIFY(
            widget.config().analysisMode ==
            doctor::domain::AnalysisMode::SourceScan);
        QVERIFY(!cmake->isEnabled());
        const auto screenshotPath =
            qEnvironmentVariable("UNITY_DOCTOR_SOURCE_SETUP_SCREENSHOT");
        if (!screenshotPath.isEmpty()) {
            widget.setStyleSheet(doctor::ui::buildUiStyleSheet(widget.palette()));
            widget.resize(1320, 840);
            widget.show();
            QTest::qWait(80);
            QVERIFY(widget.grab().save(screenshotPath));
        }
    }

    void sourceWorkspaceShowsEveryRiskInOneCheck() {
        doctor::ui::AnalysisWorkspaceWidget widget;
        widget.reset(QStringLiteral("source-fixture"), true);
        const auto issue = [](const QString& id, const QString& summary) {
            return QVariantMap{
                {QStringLiteral("id"), id},
                {QStringLiteral("ruleId"), QStringLiteral("UBD-MACRO-001")},
                {QStringLiteral("category"), QStringLiteral("SOURCE_MACRO_LEAK")},
                {QStringLiteral("severity"), QStringLiteral("warning")},
                {QStringLiteral("summary"), summary},
                {QStringLiteral("fingerprint"), id},
                {QStringLiteral("confidence"), 0.75},
                {QStringLiteral("sources"), QStringList{id + QStringLiteral(".cpp")}},
                {QStringLiteral("evidence"), QStringList{QStringLiteral("#define FLAG")}},
                {QStringLiteral("suggestion"), QStringLiteral("使用后 #undef")},
                {QStringLiteral("cmake"), QStringLiteral("# candidate")}};
        };
        widget.addTarget(QVariantMap{
            {QStringLiteral("name"), QStringLiteral("宏定义检查")},
            {QStringLiteral("type"), QStringLiteral("SOURCE_SCAN")},
            {QStringLiteral("status"), QStringLiteral("Risk Found")},
            {QStringLiteral("stage"), QStringLiteral("source-scan")},
            {QStringLiteral("issues"), QVariantList{
                 issue(QStringLiteral("first"), QStringLiteral("第一条风险")),
                 issue(QStringLiteral("second"), QStringLiteral("第二条风险"))}}});

        auto* table = widget.findChild<QTableWidget*>(
            QStringLiteral("targetsTable"));
        auto* details = widget.findChild<QTextBrowser*>(
            QStringLiteral("issueDetails"));
        auto* cmake = widget.findChild<QPlainTextEdit*>(
            QStringLiteral("cmakeSuggestion"));
        auto* detailSplitter = widget.findChild<QSplitter*>(
            QStringLiteral("detailContentSplitter"));
        auto* focus = widget.findChild<QPushButton*>(
            QStringLiteral("focusDetailsButton"));
        auto* targetsPane = widget.findChild<QFrame*>(
            QStringLiteral("targetsPane"));
        auto* logCard = widget.findChild<QFrame*>(
            QStringLiteral("logCard"));
        QVERIFY(table);
        QVERIFY(details);
        QVERIFY(cmake);
        QVERIFY(detailSplitter);
        QVERIFY(focus);
        QVERIFY(targetsPane);
        QVERIFY(logCard);
        QCOMPARE(table->horizontalHeaderItem(0)->text(), QStringLiteral("检查项"));
        QVERIFY(details->toPlainText().contains(QStringLiteral("第一条风险")));
        QVERIFY(details->toPlainText().contains(QStringLiteral("第二条风险")));
        QCOMPARE(detailSplitter->orientation(), Qt::Vertical);
        QCOMPARE(detailSplitter->count(), 2);
        QVERIFY(!detailSplitter->childrenCollapsible());

        widget.resize(1320, 840);
        widget.show();
        QTest::qWait(50);
        const auto detailsBefore = details->toPlainText();
        const auto cmakeBefore = cmake->toPlainText();
        const auto splitterSizes = detailSplitter->sizes();
        QVERIFY(splitterSizes.at(0) > splitterSizes.at(1));
        QVERIFY(splitterSizes.at(1) > 0);

        QTest::mouseClick(focus, Qt::LeftButton);
        QVERIFY(!targetsPane->isVisible());
        QVERIFY(!logCard->isVisible());
        QVERIFY(details->isVisible());
        QCOMPARE(focus->text(), QStringLiteral("退出专注"));
        const auto screenshotPath =
            qEnvironmentVariable("UNITY_DOCTOR_FOCUSED_DETAIL_SCREENSHOT");
        if (!screenshotPath.isEmpty()) {
            widget.setStyleSheet(doctor::ui::buildUiStyleSheet(widget.palette()));
            QTest::qWait(50);
            QVERIFY(widget.grab().save(screenshotPath));
        }

        QTest::mouseClick(focus, Qt::LeftButton);
        QVERIFY(targetsPane->isVisible());
        QVERIFY(logCard->isVisible());
        QCOMPARE(focus->text(), QStringLiteral("专注详情"));
        QCOMPARE(details->toPlainText(), detailsBefore);
        QCOMPARE(cmake->toPlainText(), cmakeBefore);
    }

    void projectsTargetsAndCapsVisibleLogs() {
        doctor::ui::AnalysisWorkspaceWidget widget;
        widget.reset(QStringLiteral("fixture"));
        QVariantMap issue{
            {QStringLiteral("id"), QStringLiteral("ISSUE-demo")},
            {QStringLiteral("category"), QStringLiteral("TU_LOCAL_NAME")},
            {QStringLiteral("summary"), QStringLiteral("同名 static 定义")},
            {QStringLiteral("confidence"), 0.95},
            {QStringLiteral("sources"), QStringList{
                 QStringLiteral("a.cpp"), QStringLiteral("b.cpp")}},
            {QStringLiteral("suggestion"), QStringLiteral("排除冲突集合")},
            {QStringLiteral("cmake"), QStringLiteral(
                 "set_source_files_properties(a.cpp b.cpp PROPERTIES "
                 "SKIP_UNITY_BUILD_INCLUSION ON)")}};
        QVariantMap target{
            {QStringLiteral("name"), QStringLiteral("demo")},
            {QStringLiteral("type"), QStringLiteral("STATIC_LIBRARY")},
            {QStringLiteral("status"), QStringLiteral("Unity Failed")},
            {QStringLiteral("stage"), QStringLiteral("minimized")},
            {QStringLiteral("issues"), QVariantList{issue}}};
        widget.addTarget(target);

        auto* table = widget.findChild<QTableWidget*>(
            QStringLiteral("targetsTable"));
        auto* logs = widget.findChild<QPlainTextEdit*>(
            QStringLiteral("analysisLogs"));
        QVERIFY(table);
        QVERIFY(logs);
        QCOMPARE(table->rowCount(), 1);
        QCOMPARE(table->item(0, 0)->text(), QStringLiteral("demo"));

        const auto screenshotPath =
            qEnvironmentVariable("UNITY_DOCTOR_WORKSPACE_SCREENSHOT");
        if (!screenshotPath.isEmpty()) {
            widget.setStyleSheet(doctor::ui::buildUiStyleSheet(widget.palette()));
            widget.resize(1320, 840);
            widget.setFinished(false);
            widget.show();
            QTest::qWait(80);
            QVERIFY(widget.grab().save(screenshotPath));
        }

        QString batch;
        for (int index = 0; index < 10050; ++index) {
            batch += QStringLiteral("line %1\n").arg(index);
        }
        widget.appendLog(batch);
        QVERIFY(logs->document()->blockCount() <= 10000);
    }

    void controllerStreamsEveryTargetFromWorkerThread() {
        QTemporaryDir work;
        QVERIFY(work.isValid());
        doctor::domain::ProjectConfig config;
        config.sourceDirectory = TEST_FIXTURE_DIRECTORY;
        config.workDirectory = work.path().toStdString();
        config.cmakeExecutable = TEST_CMAKE_COMMAND;
        config.generator = TEST_CMAKE_GENERATOR;
        config.parallelJobs = 2;
        config.maxProbes = 40;
        config.timeoutSeconds = 60;

        doctor::ui::AnalysisController controller;
        QSignalSpy targetSpy(
            &controller, &doctor::ui::AnalysisController::targetReceived);
        QSignalSpy finishSpy(
            &controller, &doctor::ui::AnalysisController::analysisFinished);
        controller.start(config);
        QVERIFY2(finishSpy.wait(60000), "后台分析未在 60 秒内结束");
        QCOMPARE(targetSpy.count(), 3);
        QCOMPARE(finishSpy.count(), 1);
        QVERIFY(QFile::exists(work.filePath(QStringLiteral("session.json"))));
        QVERIFY(QFile::exists(work.filePath(QStringLiteral("analysis.log"))));
    }

};

QTEST_MAIN(UiTest)
#include "ui_test.moc"
