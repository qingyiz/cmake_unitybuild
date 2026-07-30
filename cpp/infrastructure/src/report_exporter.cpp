#include "doctor/infrastructure/report_exporter.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace doctor::infrastructure {
namespace {

QJsonObject issueJson(const doctor::domain::Issue& issue) {
    QJsonObject value{
        {QStringLiteral("id"), QString::fromStdString(issue.id)},
        {QStringLiteral("target"), QString::fromStdString(issue.target)},
        {QStringLiteral("category"), QString::fromStdString(issue.category)},
        {QStringLiteral("severity"), QString::fromStdString(issue.severity)},
        {QStringLiteral("summary"), QString::fromStdString(issue.summary)},
        {QStringLiteral("fingerprint"), QString::fromStdString(issue.fingerprint)},
        {QStringLiteral("suggestion"), QString::fromStdString(issue.suggestion)},
        {QStringLiteral("cmake"), QString::fromStdString(issue.cmakeSnippet)},
        {QStringLiteral("confidence"), issue.confidence}};
    QJsonArray sources;
    for (const auto& source : issue.sources) {
        sources.push_back(QString::fromStdString(source));
    }
    QJsonArray evidence;
    for (const auto& item : issue.evidence) {
        evidence.push_back(QString::fromStdString(item));
    }
    value.insert(QStringLiteral("sources"), sources);
    value.insert(QStringLiteral("evidence"), evidence);
    return value;
}

QJsonObject sessionJson(const doctor::domain::ProjectSession& session) {
    QJsonArray targets;
    for (const auto& target : session.targets) {
        QJsonArray issues;
        for (const auto& issue : target.issues) {
            issues.push_back(issueJson(issue));
        }
        targets.push_back(QJsonObject{
            {QStringLiteral("name"), QString::fromStdString(target.name)},
            {QStringLiteral("type"), QString::fromStdString(target.type)},
            {QStringLiteral("status"), QString::fromStdString(toString(target.status))},
            {QStringLiteral("stage"), QString::fromStdString(target.stage)},
            {QStringLiteral("issues"), issues}});
    }
    return QJsonObject{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("sourceDirectory"),
         QString::fromStdString(session.sourceDirectory)},
        {QStringLiteral("workDirectory"),
         QString::fromStdString(session.workDirectory)},
        {QStringLiteral("cancelled"), session.cancelled},
        {QStringLiteral("targets"), targets}};
}

bool writeAtomic(const QString& path, const QByteArray& bytes, QString* error) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}

QString markdownFrom(const QJsonObject& session) {
    QString output = QStringLiteral(
        "# Unity Build Doctor 报告\n\n"
        "- 项目：`%1`\n"
        "- 状态：%2\n\n"
        "## Target 结果\n\n"
        "| Target | 类型 | 状态 | 问题数 |\n"
        "|---|---|---|---:|\n")
        .arg(session.value(QStringLiteral("sourceDirectory")).toString(),
             session.value(QStringLiteral("cancelled")).toBool()
                 ? QStringLiteral("已取消（部分结果）")
                 : QStringLiteral("完成"));
    for (const auto& targetValue : session.value(QStringLiteral("targets")).toArray()) {
        const auto target = targetValue.toObject();
        output += QStringLiteral("| `%1` | %2 | %3 | %4 |\n")
            .arg(target.value(QStringLiteral("name")).toString(),
                 target.value(QStringLiteral("type")).toString(),
                 target.value(QStringLiteral("status")).toString(),
                 QString::number(target.value(QStringLiteral("issues")).toArray().size()));
    }
    output += QStringLiteral("\n## 问题详情\n");
    for (const auto& targetValue : session.value(QStringLiteral("targets")).toArray()) {
        const auto target = targetValue.toObject();
        for (const auto& issueValue : target.value(QStringLiteral("issues")).toArray()) {
            const auto issue = issueValue.toObject();
            output += QStringLiteral(
                "\n### %1 · %2\n\n"
                "%3\n\n"
                "- 置信度：%4%\n"
                "- 建议：%5\n"
                "- 最小冲突文件：\n")
                .arg(issue.value(QStringLiteral("id")).toString(),
                     issue.value(QStringLiteral("category")).toString(),
                     issue.value(QStringLiteral("summary")).toString(),
                     QString::number(
                         issue.value(QStringLiteral("confidence")).toDouble() * 100.0,
                         'f', 0),
                     issue.value(QStringLiteral("suggestion")).toString());
            for (const auto& source : issue.value(QStringLiteral("sources")).toArray()) {
                output += QStringLiteral("  - `%1`\n").arg(source.toString());
            }
        }
    }
    return output;
}

QString cmakeFrom(const QJsonObject& session) {
    QString output = QStringLiteral(
        "# Unity Build Doctor 生成的候选修复\n"
        "# 请人工审查后合并到项目 CMake 配置；本文件不会自动修改源码树。\n\n");
    for (const auto& targetValue : session.value(QStringLiteral("targets")).toArray()) {
        const auto target = targetValue.toObject();
        for (const auto& issueValue : target.value(QStringLiteral("issues")).toArray()) {
            const auto issue = issueValue.toObject();
            const auto snippet = issue.value(QStringLiteral("cmake")).toString();
            if (!snippet.isEmpty()) {
                output += QStringLiteral("# %1 · %2\n%3\n\n")
                    .arg(issue.value(QStringLiteral("id")).toString(),
                         issue.value(QStringLiteral("category")).toString(),
                         snippet);
            }
        }
    }
    return output;
}

}  // namespace

bool ReportExporter::saveSession(
    const doctor::domain::ProjectSession& session,
    QString* error) const {
    const auto directory = QString::fromStdString(session.workDirectory);
    if (!QDir().mkpath(directory)) {
        if (error) {
            *error = QStringLiteral("无法创建会话目录：%1").arg(directory);
        }
        return false;
    }
    return writeAtomic(
        QDir(directory).filePath(QStringLiteral("session.json")),
        QJsonDocument(sessionJson(session)).toJson(QJsonDocument::Indented),
        error);
}

bool ReportExporter::exportAll(
    const QString& workDirectory,
    const QString& destinationDirectory,
    QString* error) const {
    QFile sessionFile(QDir(workDirectory).filePath(QStringLiteral("session.json")));
    if (!sessionFile.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("无法读取当前会话：%1").arg(sessionFile.errorString());
        }
        return false;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(sessionFile.readAll(), &parseError);
    if (!document.isObject() ||
        document.object().value(QStringLiteral("schemaVersion")).toInt() != 1) {
        if (error) {
            *error = QStringLiteral("会话文件无效：%1").arg(parseError.errorString());
        }
        return false;
    }
    if (!QDir().mkpath(destinationDirectory)) {
        if (error) {
            *error = QStringLiteral("无法创建导出目录。");
        }
        return false;
    }
    const auto session = document.object();
    const auto root = QDir(destinationDirectory);
    return writeAtomic(
               root.filePath(QStringLiteral("unity-build-doctor.json")),
               QJsonDocument(session).toJson(QJsonDocument::Indented), error) &&
        writeAtomic(
               root.filePath(QStringLiteral("unity-build-doctor.md")),
               markdownFrom(session).toUtf8(), error) &&
        writeAtomic(
               root.filePath(QStringLiteral("unity-build-doctor.cmake")),
               cmakeFrom(session).toUtf8(), error);
}

}  // namespace doctor::infrastructure
