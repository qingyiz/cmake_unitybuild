#include "doctor/domain/source_scan.h"

#include "source_scan_internal.h"

#include <algorithm>
#include <map>
#include <set>

namespace doctor::domain {
namespace {

using source_scan_internal::Definition;
using source_scan_internal::FileFacts;

Issue fileIssue(
    int sequence,
    const std::string& ruleId,
    const std::string& target,
    const std::string& category,
    const std::string& summary,
    const std::string& suggestion,
    const std::string& path,
    const std::vector<Definition>& definitions,
    double confidence) {
    Issue issue;
    issue.id = "SOURCE-" + std::to_string(sequence);
    issue.ruleId = ruleId;
    issue.target = target;
    issue.category = category;
    issue.severity = "warning";
    issue.summary = summary;
    issue.fingerprint = "source-scan|" + ruleId + "|" + path;
    issue.sources = {path};
    for (const auto& definition : definitions) {
        issue.evidence.push_back(definition.evidence);
    }
    issue.suggestion = suggestion;
    issue.cmakeSnippet = source_scan_internal::skipSnippet(issue.sources);
    issue.confidence = confidence;
    return issue;
}

Issue crossFileIssue(
    int sequence,
    const std::string& ruleId,
    const std::string& target,
    const std::string& category,
    const std::string& summary,
    const std::string& suggestion,
    const std::vector<Definition>& definitions,
    const std::set<std::string>& paths,
    double confidence) {
    Issue issue;
    issue.id = "SOURCE-" + std::to_string(sequence);
    issue.ruleId = ruleId;
    issue.target = target;
    issue.category = category;
    issue.severity = "error";
    issue.summary = summary;
    issue.fingerprint = "source-scan|" + ruleId + "|" + definitions.front().name;
    issue.sources.assign(paths.begin(), paths.end());
    for (const auto& definition : definitions) {
        issue.evidence.push_back(definition.evidence);
    }
    issue.suggestion = suggestion;
    issue.cmakeSnippet = source_scan_internal::skipSnippet(issue.sources);
    issue.confidence = confidence;
    return issue;
}

}  // namespace

std::vector<Issue> scanSourceRisks(
    const std::vector<SourceDocument>& inputDocuments) {
    auto documents = inputDocuments;
    std::sort(documents.begin(), documents.end(), [](const auto& left, const auto& right) {
        return left.path < right.path;
    });
    std::map<std::string, FileFacts> facts;
    for (const auto& document : documents) {
        facts[document.path] = source_scan_internal::analyzeDocument(document);
    }

    std::vector<Issue> issues;
    int sequence = 0;
    std::map<std::string, std::vector<Definition>> macrosByName;
    std::map<std::string, std::vector<Definition>> staticsByName;
    for (const auto& [path, file] : facts) {
        if (!file.activeMacros.empty()) {
            issues.push_back(fileIssue(
                ++sequence, "UBD-MACRO-001", "宏定义检查", "SOURCE_MACRO_LEAK",
                "源文件结束时仍有活动宏，可能泄漏到后续 Unity 源文件。",
                "在宏使用结束后显式 #undef，或改为 constexpr、函数、枚举或命名空间常量。",
                path, file.activeMacros, 0.75));
            for (const auto& definition : file.activeMacros) {
                macrosByName[definition.name].push_back(definition);
            }
        }
        if (!file.usingNamespaces.empty()) {
            issues.push_back(fileIssue(
                ++sequence, "UBD-USING-001", "using namespace 检查",
                "FILE_USING_NAMESPACE",
                "文件作用域 using namespace 会污染同一 Unity 翻译单元中的后续源文件。",
                "使用限定名称，或把 using 声明缩小到函数/局部作用域。",
                path, file.usingNamespaces, 0.80));
        }
        for (const auto& definition : file.staticSymbols) {
            staticsByName[definition.name].push_back(definition);
        }
    }

    for (const auto& [name, definitions] : macrosByName) {
        std::set<std::string> values;
        std::set<std::string> paths;
        for (const auto& definition : definitions) {
            values.insert(definition.value);
            paths.insert(definition.path);
        }
        if (paths.size() >= 2 && values.size() >= 2) {
            issues.push_back(crossFileIssue(
                ++sequence, "UBD-MACRO-002", "宏定义检查",
                "SOURCE_MACRO_CONFLICT",
                "不同源文件中的活动同名宏具有不同替换内容：" + name,
                "统一或局部化该宏，并在文件结束前 #undef；随后使用构建验证确认真实分组。",
                definitions, paths, 0.90));
        }
    }

    for (const auto& [name, definitions] : staticsByName) {
        std::set<std::string> paths;
        for (const auto& definition : definitions) {
            paths.insert(definition.path);
        }
        if (paths.size() >= 2) {
            issues.push_back(crossFileIssue(
                ++sequence, "UBD-STATIC-001", "文件级 static 检查",
                "FILE_STATIC_COLLISION",
                "不同源文件包含同名文件作用域 static，进入同一 Unity 单元时可能重定义：" +
                    name,
                "为符号使用文件唯一名称、重构到具名命名空间，或临时排除相关源文件。",
                definitions, paths, 0.95));
        }
    }
    return issues;
}

}  // namespace doctor::domain
