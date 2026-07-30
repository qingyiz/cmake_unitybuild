#include "doctor/domain/diagnostics.h"
#include "doctor/domain/minimizer.h"
#include "doctor/domain/source_scan.h"

#include <QTest>

#include <algorithm>
#include <map>

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

    void scansSourceRisksWithoutFalseLocalScopeMatches() {
        const std::vector<doctor::domain::SourceDocument> documents{
            {"src/a.cpp",
             "#define FEATURE_FLAG 1\n"
             "using namespace alpha;\n"
             "static int cache = 1;\n"
             "void local() { using namespace inner; static int localCache = 0; }\n"},
            {"src/b.cpp",
             "#define FEATURE_FLAG 2\n"
             "static int cache = 2;\n"
             "const char* ignored = \"using namespace text; static int fake;\";\n"},
            {"src/safe.cpp",
             "#define LOCAL_ONLY 1\n"
             "#undef LOCAL_ONLY\n"
             "/*\n#define COMMENTED_MACRO 1\n*/\n"
             "// using namespace comment;\n"
             "class Holder { static int cache; };\n"}};

        const auto issues = doctor::domain::scanSourceRisks(documents);
        std::map<std::string, int> counts;
        for (const auto& issue : issues) {
            ++counts[issue.ruleId];
            QVERIFY(QString::fromStdString(issue.fingerprint)
                .startsWith(QStringLiteral("source-scan|")));
        }
        QCOMPARE(counts["UBD-MACRO-001"], 2);
        QCOMPARE(counts["UBD-MACRO-002"], 1);
        QCOMPARE(counts["UBD-USING-001"], 1);
        QCOMPARE(counts["UBD-STATIC-001"], 1);
    }

    void sourceScanOrderingIsDeterministic() {
        const std::vector<doctor::domain::SourceDocument> forward{
            {"z.cpp", "#define FLAG 2\nstatic int token;\n"},
            {"a.cpp", "#define FLAG 1\nstatic int token;\n"}};
        auto reversed = forward;
        std::reverse(reversed.begin(), reversed.end());
        const auto first = doctor::domain::scanSourceRisks(forward);
        const auto second = doctor::domain::scanSourceRisks(reversed);
        QCOMPARE(first.size(), second.size());
        for (std::size_t index = 0; index < first.size(); ++index) {
            QCOMPARE(
                QString::fromStdString(first[index].fingerprint),
                QString::fromStdString(second[index].fingerprint));
        }
    }
};

QTEST_APPLESS_MAIN(DomainTest)
#include "domain_test.moc"
