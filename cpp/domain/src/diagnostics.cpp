#include "doctor/domain/diagnostics.h"

#include <algorithm>
#include <regex>
#include <set>
#include <sstream>

namespace doctor::domain {
namespace {

bool contains(const std::string& text, const std::string& token) {
    return text.find(token) != std::string::npos;
}

int countMatching(
    const std::vector<std::string>& sources,
    const std::map<std::string, std::string>& texts,
    const std::regex& pattern) {
    int count = 0;
    for (const auto& source : sources) {
        const auto iterator = texts.find(source);
        if (iterator != texts.end() && std::regex_search(iterator->second, pattern)) {
            ++count;
        }
    }
    return count;
}

std::vector<std::string> sourceEvidence(
    const std::vector<std::string>& sources,
    const std::map<std::string, std::string>& texts,
    const std::regex& pattern) {
    std::vector<std::string> evidence;
    for (const auto& source : sources) {
        const auto iterator = texts.find(source);
        if (iterator == texts.end()) {
            continue;
        }
        std::istringstream stream(iterator->second);
        std::string line;
        int number = 0;
        while (std::getline(stream, line)) {
            ++number;
            if (std::regex_search(line, pattern)) {
                evidence.push_back(source + ":" + std::to_string(number) + ": " + line);
                break;
            }
        }
    }
    return evidence;
}

bool versionAtLeast(
    const std::tuple<int, int, int>& value,
    int major,
    int minor,
    int patch = 0) {
    return value >= std::make_tuple(major, minor, patch);
}

}  // namespace

Classification classify(
    const FailureFingerprint& fingerprint,
    const std::vector<std::string>& sources,
    const std::map<std::string, std::string>& texts,
    const std::map<std::string, std::string>& compileSignatures) {
    if (sources.size() == 1) {
        return {"SINGLE_SOURCE", "单个源文件即可复现。", sources, {}, 0.95};
    }
    for (const auto& source : sources) {
        if (contains(source, "moc_") || contains(source, "_autogen") ||
            contains(source, "qrc_") || contains(source, ".moc")) {
            return {"QT_GENERATED_SOURCE", "冲突集合包含 Qt/AUTOGEN 生成源。", {source}, {}, 0.95};
        }
    }
    if (!compileSignatures.empty()) {
        std::set<std::string> unique;
        for (const auto& source : sources) {
            const auto iterator = compileSignatures.find(source);
            if (iterator != compileSignatures.end()) {
                unique.insert(iterator->second);
            }
        }
        if (unique.size() > 1) {
            return {"PER_SOURCE_OPTIONS", "源文件的有效编译选项不同。", {}, {}, 0.95};
        }
    }
    const std::regex definePattern(R"(^\s*#\s*define\b)", std::regex::multiline);
    const std::regex consumerPattern(R"(^\s*#\s*(if|ifdef|ifndef|error)\b)", std::regex::multiline);
    if (countMatching(sources, texts, definePattern) > 0 &&
        countMatching(sources, texts, consumerPattern) > 0) {
        return {
            "MACRO_LEAK",
            "前序源文件留下的宏状态改变了后续源文件。",
            sourceEvidence(sources, texts, std::regex(R"(^\s*#)")),
            {},
            0.90};
    }
    if (fingerprint.category == "redefinition" &&
        countMatching(sources, texts, std::regex(R"(namespace\s*\{)")) >= 2) {
        return {
            "ANONYMOUS_NAMESPACE",
            "匿名命名空间在 Unity 翻译单元中合并并产生同名定义。",
            sourceEvidence(sources, texts, std::regex(R"(namespace\s*\{)")),
            {},
            0.95};
    }
    const auto staticPattern = fingerprint.symbol.empty()
        ? std::regex(R"(\bstatic\b)")
        : std::regex("\\bstatic\\b[^\\n;{}]*\\b" + fingerprint.symbol + "\\b");
    if (fingerprint.category == "redefinition" &&
        countMatching(sources, texts, staticPattern) >= 2) {
        return {
            "TU_LOCAL_NAME",
            "多个原翻译单元包含同名文件级 static 定义。",
            sourceEvidence(sources, texts, staticPattern),
            {},
            0.95};
    }
    if (fingerprint.category == "missing_declaration" ||
        fingerprint.category == "incomplete_type" ||
        fingerprint.category == "missing_include") {
        return {
            "INCLUDE_ORDER",
            "源文件依赖 include 顺序、间接包含或前置声明状态。",
            {fingerprint.message},
            {"也可能是迁移时遗漏 include directory"},
            0.80};
    }
    if (contains(fingerprint.message, ".h") && fingerprint.category == "redefinition") {
        return {
            "HEADER_DEFINITION",
            "重定义来自头文件，可能缺少保护或包含非 inline 定义。",
            {fingerprint.message},
            {},
            0.75};
    }
    return {
        "UNKNOWN",
        "现有证据不足以安全确定根因。",
        {fingerprint.message},
        {"翻译单元名字冲突", "宏或 include 顺序", "生成源差异"},
        0.35};
}

std::vector<Issue> buildIssues(
    const std::string& target,
    const FailureFingerprint& fingerprint,
    const std::vector<std::string>& minimalSources,
    const std::map<std::string, std::string>& sourceTexts,
    const std::tuple<int, int, int>& cmakeVersion) {
    const auto classification = classify(fingerprint, minimalSources, sourceTexts);
    Issue issue;
    issue.id = "ISSUE-" + target;
    issue.target = target;
    issue.category = classification.category;
    issue.summary = classification.summary;
    issue.fingerprint = fingerprint.key();
    issue.sources = minimalSources;
    issue.evidence = classification.evidence;
    issue.confidence = classification.confidence;

    std::ostringstream cmake;
    if (versionAtLeast(cmakeVersion, 3, 16)) {
        cmake << "set_source_files_properties(\n";
        for (const auto& source : minimalSources) {
            cmake << "    \"" << source << "\"\n";
        }
        if (versionAtLeast(cmakeVersion, 3, 18)) {
            cmake << "    TARGET_DIRECTORY \"" << target << "\"\n";
        }
        cmake << "    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON\n)";
    }
    issue.cmakeSnippet = cmake.str();
    issue.suggestion = "优先排除最小冲突集合；修复根因后再让文件进入 Unity。";
    return {issue};
}

}  // namespace doctor::domain
