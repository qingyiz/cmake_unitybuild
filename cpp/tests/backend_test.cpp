#include "doctor/application/analysis_service.h"
#include "doctor/infrastructure/cmake_backend.h"
#include "doctor/infrastructure/report_exporter.h"
#include "doctor/infrastructure/source_scan_backend.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <atomic>
#include <numeric>

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

    void scansSourcesWithoutConfiguringOrBuilding() {
        QTemporaryDir work;
        QTemporaryDir reports;
        QVERIFY(work.isValid());
        QVERIFY(reports.isValid());

        doctor::domain::ProjectConfig config;
        config.analysisMode = doctor::domain::AnalysisMode::SourceScan;
        config.sourceDirectory = TEST_SOURCE_SCAN_DIRECTORY;
        config.workDirectory = work.path().toStdString();
        config.cmakeExecutable = "/definitely/not/a/cmake/executable";

        doctor::infrastructure::CMakeBackend buildBackend;
        doctor::infrastructure::SourceScanBackend sourceBackend;
        doctor::application::ProjectAnalysisService service(
            buildBackend, buildBackend, &sourceBackend);
        std::atomic_bool cancelled{false};
        const auto session = service.run(
            config,
            cancelled,
            [](const auto&) {},
            [](const auto&) {},
            [](const auto&) {});

        QCOMPARE(session.analysisMode, std::string("source-scan"));
        QCOMPARE(session.targets.size(), std::size_t{3});
        QVERIFY(!QDir(work.path()).exists(QStringLiteral("baseline")));
        QVERIFY(!QDir(work.path()).exists(QStringLiteral("unity")));
        QVERIFY(!QDir(work.path()).exists(QStringLiteral("probes")));
        for (const auto& target : session.targets) {
            QCOMPARE(target.type, std::string("SOURCE_SCAN"));
            QVERIFY(target.status == doctor::domain::TargetStatus::RiskFound);
        }
        const auto issueCount = std::accumulate(
            session.targets.begin(), session.targets.end(), std::size_t{0},
            [](std::size_t total, const auto& target) {
                return total + target.issues.size();
            });
        QCOMPARE(issueCount, std::size_t{6});

        doctor::infrastructure::ReportExporter exporter;
        QString error;
        QVERIFY2(exporter.saveSession(session, &error), qPrintable(error));
        QFile json(work.filePath(QStringLiteral("session.json")));
        QVERIFY(json.open(QIODevice::ReadOnly));
        const auto object = QJsonDocument::fromJson(json.readAll()).object();
        QCOMPARE(
            object.value(QStringLiteral("analysisMode")).toString(),
            QStringLiteral("source-scan"));
        const auto firstIssue = object.value(QStringLiteral("targets")).toArray()
            .first().toObject().value(QStringLiteral("issues")).toArray()
            .first().toObject();
        QVERIFY(firstIssue.value(QStringLiteral("ruleId")).toString()
            .startsWith(QStringLiteral("UBD-")));
        QVERIFY2(
            exporter.exportAll(work.path(), reports.path(), &error),
            qPrintable(error));
        QFile markdown(
            reports.filePath(QStringLiteral("unity-build-doctor.md")));
        QVERIFY(markdown.open(QIODevice::ReadOnly));
        const auto markdownText = QString::fromUtf8(markdown.readAll());
        QVERIFY(markdownText.contains(QStringLiteral("source-scan")));
        QVERIFY(markdownText.contains(QStringLiteral("UBD-MACRO-001")));
    }

    void sourceScannerSkipsExcludedAndOversizedFilesAndCancels() {
        QTemporaryDir project;
        QVERIFY(project.isValid());
        QVERIFY(QDir().mkpath(project.filePath(QStringLiteral("src"))));
        QVERIFY(QDir().mkpath(project.filePath(QStringLiteral("build"))));
        const auto writeFile = [](const QString& path, const QByteArray& content) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly) &&
                file.write(content) == content.size();
        };
        QVERIFY(writeFile(
            project.filePath(QStringLiteral("src/visible.cpp")),
            QByteArray("#define VISIBLE 1\n")));
        QVERIFY(writeFile(
            project.filePath(QStringLiteral("build/excluded.cpp")),
            QByteArray("using namespace excluded;\n")));
        QVERIFY(writeFile(
            project.filePath(QStringLiteral("src/large.cpp")),
            QByteArray(2 * 1024 * 1024 + 1, 'x')));
        const auto linkPath = project.filePath(QStringLiteral("src/link.cpp"));
        const bool linkCreated = QFile::link(
            project.filePath(QStringLiteral("src/visible.cpp")), linkPath);

        doctor::domain::ProjectConfig config;
        config.analysisMode = doctor::domain::AnalysisMode::SourceScan;
        config.sourceDirectory = project.path().toStdString();
        doctor::infrastructure::SourceScanBackend backend;
        std::atomic_bool cancelled{false};
        QStringList logs;
        const auto inventory = backend.scan(
            config,
            cancelled,
            [&](const std::string& line) {
                logs.push_back(QString::fromStdString(line));
            });
        QVERIFY(inventory.valid);
        QCOMPARE(inventory.targets.size(), std::size_t{3});
        QString combinedSources;
        for (const auto& target : inventory.targets) {
            for (const auto& issue : target.issues) {
                for (const auto& source : issue.sources) {
                    combinedSources += QString::fromStdString(source);
                }
            }
        }
        QVERIFY(combinedSources.contains(QStringLiteral("visible.cpp")));
        QVERIFY(!combinedSources.contains(QStringLiteral("excluded.cpp")));
        QVERIFY(!combinedSources.contains(QStringLiteral("large.cpp")));
        QVERIFY(logs.join('\n').contains(QStringLiteral("超过 2 MiB")));
        if (linkCreated) {
            QVERIFY(logs.join('\n').contains(QStringLiteral("符号链接")));
        }

        cancelled.store(true);
        const auto cancelledInventory = backend.scan(
            config, cancelled, [](const auto&) {});
        QVERIFY(!cancelledInventory.valid);
        QVERIFY(cancelledInventory.error.find("取消") != std::string::npos);
    }

    void bundledDemoCoversEverySourceScanCheck() {
        doctor::domain::ProjectConfig config;
        config.analysisMode = doctor::domain::AnalysisMode::SourceScan;
        config.sourceDirectory = TEST_DEMO_DIRECTORY;
        doctor::infrastructure::SourceScanBackend backend;
        std::atomic_bool cancelled{false};
        const auto inventory = backend.scan(
            config, cancelled, [](const auto&) {});

        QVERIFY(inventory.valid);
        QCOMPARE(inventory.targets.size(), std::size_t{3});
        std::size_t issueCount = 0;
        for (const auto& target : inventory.targets) {
            QVERIFY(target.status == doctor::domain::TargetStatus::RiskFound);
            issueCount += target.issues.size();
        }
        QCOMPARE(issueCount, std::size_t{5});
    }
};

QTEST_GUILESS_MAIN(BackendTest)
#include "backend_test.moc"
