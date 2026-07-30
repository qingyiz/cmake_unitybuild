#include "doctor/infrastructure/source_scan_backend.h"

#include "doctor/domain/source_scan.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

#include <array>

namespace doctor::infrastructure {
namespace {

constexpr qint64 kMaximumSourceBytes = 2 * 1024 * 1024;

bool isSourceFile(const QFileInfo& info) {
    static const QSet<QString> extensions{
        QStringLiteral("c"), QStringLiteral("cc"), QStringLiteral("cpp"),
        QStringLiteral("cxx"), QStringLiteral("m"), QStringLiteral("mm")};
    return extensions.contains(info.suffix().toLower());
}

bool shouldSkipDirectory(const QString& name) {
    static const QSet<QString> excluded{
        QStringLiteral(".git"), QStringLiteral(".svn"), QStringLiteral(".hg"),
        QStringLiteral("build"), QStringLiteral("out"), QStringLiteral("dist")};
    const auto lower = name.toLower();
    return excluded.contains(lower) ||
        lower.startsWith(QStringLiteral("cmake-build-"));
}

void collectDocuments(
    const QDir& root,
    const QDir& directory,
    std::atomic_bool& cancelled,
    const doctor::application::LogSink& log,
    std::vector<doctor::domain::SourceDocument>* documents) {
    const auto entries = directory.entryInfoList(
        QDir::AllDirs | QDir::Files | QDir::Hidden | QDir::System |
            QDir::NoDotAndDotDot,
        QDir::DirsFirst | QDir::Name);
    for (const auto& entry : entries) {
        if (cancelled.load()) {
            return;
        }
        if (entry.isSymLink()) {
            log("跳过符号链接：" + entry.absoluteFilePath().toStdString());
            continue;
        }
        if (entry.isDir()) {
            if (!shouldSkipDirectory(entry.fileName())) {
                collectDocuments(
                    root, QDir(entry.absoluteFilePath()), cancelled, log, documents);
            }
            continue;
        }
        if (!entry.isFile() || !isSourceFile(entry)) {
            continue;
        }
        if (entry.size() > kMaximumSourceBytes) {
            log("跳过超过 2 MiB 的源码：" + entry.absoluteFilePath().toStdString());
            continue;
        }
        QFile file(entry.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            log("无法读取源码，已跳过：" + entry.absoluteFilePath().toStdString());
            continue;
        }
        const auto relativePath =
            QDir::fromNativeSeparators(root.relativeFilePath(entry.absoluteFilePath()));
        documents->push_back(
            {relativePath.toStdString(), file.readAll().toStdString()});
    }
}

doctor::domain::TargetResult makeCheck(
    const std::string& name,
    const std::vector<doctor::domain::Issue>& allIssues) {
    doctor::domain::TargetResult result;
    result.name = name;
    result.type = "SOURCE_SCAN";
    result.stage = "source-scan";
    for (const auto& issue : allIssues) {
        if (issue.target == name) {
            result.issues.push_back(issue);
        }
    }
    result.status = result.issues.empty()
        ? doctor::domain::TargetStatus::Passed
        : doctor::domain::TargetStatus::RiskFound;
    return result;
}

}  // namespace

doctor::application::ProjectInventory SourceScanBackend::scan(
    const doctor::domain::ProjectConfig& config,
    std::atomic_bool& cancelled,
    const doctor::application::LogSink& log) {
    doctor::application::ProjectInventory inventory;
    const QDir root(QString::fromStdString(config.sourceDirectory));
    if (!root.exists()) {
        inventory.error = "项目目录不存在";
        return inventory;
    }

    std::vector<doctor::domain::SourceDocument> documents;
    collectDocuments(root, root, cancelled, log, &documents);
    if (cancelled.load()) {
        inventory.error = "源码扫描已取消";
        return inventory;
    }
    if (documents.empty()) {
        inventory.error = "项目目录中没有可扫描的 C/C++/Objective-C 源文件";
        return inventory;
    }

    log("已读取 " + std::to_string(documents.size()) + " 个源文件，开始规则匹配");
    const auto issues = doctor::domain::scanSourceRisks(documents);
    for (const auto& name : std::array<std::string, 3>{
             "宏定义检查", "using namespace 检查", "文件级 static 检查"}) {
        inventory.targets.push_back(makeCheck(name, issues));
    }
    inventory.valid = true;
    log("源码扫描完成，发现 " + std::to_string(issues.size()) + " 条风险");
    return inventory;
}

}  // namespace doctor::infrastructure
