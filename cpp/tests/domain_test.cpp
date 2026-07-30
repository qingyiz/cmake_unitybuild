#include "doctor/domain/diagnostics.h"
#include "doctor/domain/minimizer.h"

#include <QTest>

class DomainTest final : public QObject {
    Q_OBJECT

private slots:
    void minimizesOrderedThreeFileInteraction() {
        const std::vector<std::string> required{"macro.cpp", "bridge.cpp", "consumer.cpp"};
        const auto result = doctor::domain::minimizeOrdered(
            {"noise.cpp", "macro.cpp", "bridge.cpp", "consumer.cpp", "tail.cpp"},
            [&](const auto& sources) {
                auto position = std::size_t{0};
                for (const auto& source : sources) {
                    if (position < required.size() && source == required[position]) {
                        ++position;
                    }
                }
                return position == required.size();
            },
            100);
        QCOMPARE(
            static_cast<int>(result.status),
            static_cast<int>(doctor::domain::MinimizationResult::Status::Minimized));
        QCOMPARE(result.sources.size(), std::size_t{3});
        QCOMPARE(QString::fromStdString(result.sources.front()), QStringLiteral("macro.cpp"));
    }

    void classifiesKnownPatterns_data() {
        QTest::addColumn<QString>("expected");
        QTest::addColumn<QString>("first");
        QTest::addColumn<QString>("second");
        QTest::addColumn<QString>("category");
        QTest::addColumn<QString>("symbol");

        QTest::newRow("static") << "TU_LOCAL_NAME"
            << "static int value;" << "static int value;" << "redefinition" << "value";
        QTest::newRow("anonymous") << "ANONYMOUS_NAMESPACE"
            << "namespace { int value; }" << "namespace { int value; }"
            << "redefinition" << "value";
        QTest::newRow("macro") << "MACRO_LEAK"
            << "#define FLAG 1" << "#ifdef FLAG\n#error leak\n#endif"
            << "compiler_error" << "";
        QTest::newRow("include") << "INCLUDE_ORDER"
            << "int a;" << "Type value;" << "missing_declaration" << "Type";
        QTest::newRow("unknown") << "UNKNOWN"
            << "int a;" << "int b;" << "compiler_error" << "";
    }

    void classifiesKnownPatterns() {
        QFETCH(QString, expected);
        QFETCH(QString, first);
        QFETCH(QString, second);
        QFETCH(QString, category);
        QFETCH(QString, symbol);
        doctor::domain::FailureFingerprint fingerprint{
            "clang", "compile", category.toStdString(), symbol.toStdString(), category.toStdString()};
        const auto result = doctor::domain::classify(
            fingerprint,
            {"a.cpp", "b.cpp"},
            {{"a.cpp", first.toStdString()}, {"b.cpp", second.toStdString()}});
        QCOMPARE(QString::fromStdString(result.category), expected);
    }

    void generatesVersionScopedCMake() {
        doctor::domain::FailureFingerprint fingerprint{
            "clang", "compile", "redefinition", "value", "redefinition"};
        const auto issues = doctor::domain::buildIssues(
            "app",
            fingerprint,
            {"a.cpp", "b.cpp"},
            {{"a.cpp", "static int value;"}, {"b.cpp", "static int value;"}},
            {3, 27, 1});
        QCOMPARE(issues.size(), std::size_t{1});
        QVERIFY(QString::fromStdString(issues.front().cmakeSnippet)
            .contains("TARGET_DIRECTORY \"app\""));
    }
};

QTEST_APPLESS_MAIN(DomainTest)
#include "domain_test.moc"
