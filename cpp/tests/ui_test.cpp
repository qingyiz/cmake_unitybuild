#include "doctor/ui/analysis_controller.h"
#include "doctor/ui/analysis_workspace_widget.h"
#include "doctor/ui/project_setup_widget.h"
#include "doctor/ui/ui_theme.h"

#include <QFile>
#include <QFrame>
#include <QLineEdit>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
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
