#include "doctor/application/analysis_service.h"
#include "doctor/infrastructure/cmake_backend.h"
#include "doctor/infrastructure/report_exporter.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <atomic>

class BackendTest final : public QObject {
    Q_OBJECT

private slots:
    void analyzesEveryTargetAndExportsConsistentReports() {
        QTemporaryDir work;
        QTemporaryDir reports;
        QVERIFY(work.isValid());
        QVERIFY(reports.isValid());

        doctor::domain::ProjectConfig config;
        config.sourceDirectory = TEST_FIXTURE_DIRECTORY;
        config.workDirectory = work.path().toStdString();
        config.cmakeExecutable = TEST_CMAKE_COMMAND;
        config.generator = TEST_CMAKE_GENERATOR;
        config.parallelJobs = 2;
        config.maxProbes = 40;
        config.timeoutSeconds = 60;

        doctor::infrastructure::CMakeBackend backend;
        doctor::application::ProjectAnalysisService service(backend, backend);
        std::atomic_bool cancelled{false};
        const auto session = service.run(
            config,
            cancelled,
            [](const auto&) {},
            [](const auto&) {},
            [](const auto&) {});

        QCOMPARE(session.targets.size(), std::size_t{3});
        const auto findTarget = [&](const std::string& name)
            -> const doctor::domain::TargetResult* {
            const auto found = std::find_if(
                session.targets.begin(), session.targets.end(),
                [&](const auto& target) { return target.name == name; });
            return found == session.targets.end() ? nullptr : &*found;
        };
        const auto* good = findTarget("good_target");
        const auto* baselineBad = findTarget("baseline_bad");
        const auto* conflict = findTarget("unity_conflict");
        QVERIFY(good);
        QVERIFY(baselineBad);
        QVERIFY(conflict);
        QVERIFY(good->status == doctor::domain::TargetStatus::Passed);
        QVERIFY(
            baselineBad->status ==
            doctor::domain::TargetStatus::BaselineFailed);
        QVERIFY(conflict->status == doctor::domain::TargetStatus::UnityFailed);
        QCOMPARE(conflict->issues.size(), std::size_t{1});
        QCOMPARE(conflict->issues.front().sources.size(), std::size_t{2});
        QCOMPARE(
            conflict->issues.front().category,
            std::string("TU_LOCAL_NAME"));

        doctor::infrastructure::ReportExporter exporter;
        QString error;
        QVERIFY2(exporter.saveSession(session, &error), qPrintable(error));
        QVERIFY2(
            exporter.exportAll(work.path(), reports.path(), &error),
            qPrintable(error));
        for (const auto& name : {
                 QStringLiteral("unity-build-doctor.json"),
                 QStringLiteral("unity-build-doctor.md"),
                 QStringLiteral("unity-build-doctor.cmake")}) {
            QVERIFY(QFile::exists(reports.filePath(name)));
        }
        QFile json(reports.filePath(QStringLiteral("unity-build-doctor.json")));
        QVERIFY(json.open(QIODevice::ReadOnly));
        const auto object = QJsonDocument::fromJson(json.readAll()).object();
        QCOMPARE(object.value(QStringLiteral("targets")).toArray().size(), 3);
    }
};

QTEST_GUILESS_MAIN(BackendTest)
#include "backend_test.moc"
