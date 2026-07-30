#include "doctor/infrastructure/cmake_backend.h"

#include "doctor/domain/diagnostics.h"
#include "doctor/domain/minimizer.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QRegularExpression>

#include <algorithm>
#include <map>

namespace doctor::infrastructure {
namespace {

QStringList toQStringList(const std::vector<std::string>& values) {
    QStringList result;
    for (const auto& value : values) {
        result.push_back(QString::fromStdString(value));
    }
    return result;
}

void writeFileApiQuery(const QString& buildDirectory) {
    QDir directory(buildDirectory);
    directory.mkpath(QStringLiteral(".cmake/api/v1/query/client-unity-build-doctor"));
    QFile query(directory.filePath(
        QStringLiteral(".cmake/api/v1/query/client-unity-build-doctor/codemodel-v2")));
    query.open(QIODevice::WriteOnly);
}

ProcessResult configure(
    const ProcessRunner& runner,
    const doctor::domain::ProjectConfig& config,
    const QString& buildDirectory,
    bool unity,
    std::atomic_bool& cancelled,
    const doctor::application::LogSink& sink) {
    QDir().mkpath(buildDirectory);
    writeFileApiQuery(buildDirectory);
    QStringList arguments{
        QStringLiteral("-S"), QString::fromStdString(config.sourceDirectory),
        QStringLiteral("-B"), buildDirectory,
        QStringLiteral("-DCMAKE_UNITY_BUILD=%1").arg(unity ? "ON" : "OFF"),
        QStringLiteral("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")};
    if (!config.generator.empty()) {
        arguments << QStringLiteral("-G") << QString::fromStdString(config.generator);
    }
    arguments << toQStringList(config.cmakeArguments);
    return runner.run(
        QString::fromStdString(config.cmakeExecutable),
        arguments,
        QString::fromStdString(config.sourceDirectory),
        config.timeoutSeconds,
        cancelled,
        [&](const QString& text) { sink(text.toStdString()); });
}

QJsonObject readJsonObject(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

std::vector<doctor::domain::TargetResult> readTargets(const QString& buildDirectory) {
    QDir reply(QDir(buildDirectory).filePath(QStringLiteral(".cmake/api/v1/reply")));
    const auto indexes = reply.entryList(
        {QStringLiteral("index-*.json")}, QDir::Files, QDir::Name);
    if (indexes.isEmpty()) {
        return {};
    }
    const auto index = readJsonObject(reply.filePath(indexes.last()));
    QString codemodelFile;
    for (const auto& value : index.value(QStringLiteral("objects")).toArray()) {
        const auto object = value.toObject();
        if (object.value(QStringLiteral("kind")).toString() == QStringLiteral("codemodel")) {
            codemodelFile = object.value(QStringLiteral("jsonFile")).toString();
            break;
        }
    }
    const auto codemodel = readJsonObject(reply.filePath(codemodelFile));
    const auto configurations = codemodel.value(QStringLiteral("configurations")).toArray();
    if (configurations.isEmpty()) {
        return {};
    }
    std::vector<doctor::domain::TargetResult> targets;
    for (const auto& value : configurations.first().toObject()
             .value(QStringLiteral("targets")).toArray()) {
        const auto reference = value.toObject();
        const auto detail = readJsonObject(
            reply.filePath(reference.value(QStringLiteral("jsonFile")).toString()));
        const auto type = detail.value(QStringLiteral("type")).toString();
        static const QStringList buildable{
            "EXECUTABLE", "STATIC_LIBRARY", "SHARED_LIBRARY", "MODULE_LIBRARY", "OBJECT_LIBRARY"};
        if (!buildable.contains(type)) {
            continue;
        }
        doctor::domain::TargetResult target;
        target.name = reference.value(QStringLiteral("name")).toString().toStdString();
        target.type = type.toStdString();
        targets.push_back(std::move(target));
    }
    return targets;
}

ProcessResult buildTarget(
    const ProcessRunner& runner,
    const doctor::domain::ProjectConfig& config,
    const std::string& buildDirectory,
    const std::string& target,
    std::atomic_bool& cancelled,
    const doctor::application::LogSink& sink) {
    QStringList arguments{
        QStringLiteral("--build"), QString::fromStdString(buildDirectory),
        QStringLiteral("--config"), QString::fromStdString(config.configuration),
        QStringLiteral("--target"), QString::fromStdString(target)};
    if (config.parallelJobs > 0) {
        arguments << QStringLiteral("--parallel") << QString::number(config.parallelJobs);
    }
    return runner.run(
        QString::fromStdString(config.cmakeExecutable),
        arguments,
        QString::fromStdString(config.sourceDirectory),
        config.timeoutSeconds,
        cancelled,
        [&](const QString& text) { sink(text.toStdString()); });
}

doctor::domain::FailureFingerprint fingerprint(const QString& output) {
    static const QRegularExpression pattern(
        QStringLiteral(R"((.+?):(\d+)(?::(\d+))?:\s+(?:fatal error|error):\s+(.+))"));
    const auto match = pattern.match(output);
    doctor::domain::FailureFingerprint result;
    result.compilerFamily = output.contains(QStringLiteral("clang"), Qt::CaseInsensitive)
        ? "clang" : "unknown";
    result.category = "compiler_error";
    result.message = match.hasMatch() ? match.captured(4).toStdString() : "build failed";
    const auto message = QString::fromStdString(result.message);
    if (message.contains(QStringLiteral("redefinition"), Qt::CaseInsensitive)) {
        result.category = "redefinition";
    } else if (message.contains(QStringLiteral("undeclared"), Qt::CaseInsensitive)) {
        result.category = "missing_declaration";
    } else if (message.contains(QStringLiteral("incomplete type"), Qt::CaseInsensitive)) {
        result.category = "incomplete_type";
    } else if (message.contains(QStringLiteral("file not found"), Qt::CaseInsensitive)) {
        result.category = "missing_include";
    }
    static const QRegularExpression symbolPattern(QStringLiteral("['‘“`]([^'’”`]+)['’”`]"));
    const auto symbol = symbolPattern.match(message);
    if (symbol.hasMatch()) {
        result.symbol = symbol.captured(1).toStdString();
    }
    return result;
}

struct UnityCompileUnit {
    QString compiler;
    QStringList arguments;
    QString directory;
    QString source;
    std::vector<std::string> sources;
};

std::vector<UnityCompileUnit> unityCompileUnits(
    const QString& buildDirectory,
    const QString& target) {
    QFile file(QDir(buildDirectory).filePath(QStringLiteral("compile_commands.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    std::vector<UnityCompileUnit> units;
    const auto entries = QJsonDocument::fromJson(file.readAll()).array();
    for (const auto& value : entries) {
        const auto entry = value.toObject();
        const auto path = entry.value(QStringLiteral("file")).toString();
        if (!path.contains(QStringLiteral("/CMakeFiles/%1.dir/Unity/").arg(target))) {
            continue;
        }
        QFile unity(path);
        if (!unity.open(QIODevice::ReadOnly)) {
            continue;
        }
        const auto text = QString::fromUtf8(unity.readAll());
        static const QRegularExpression includePattern(
            QStringLiteral(R"re(^\s*#\s*include\s+"([^"]+\.(?:c|cc|cpp|cxx|m|mm))")re"),
            QRegularExpression::MultilineOption);
        UnityCompileUnit unit;
        unit.directory = entry.value(QStringLiteral("directory")).toString();
        unit.source = path;
        auto iterator = includePattern.globalMatch(text);
        while (iterator.hasNext()) {
            const auto include = iterator.next().captured(1);
            const auto absolute = QFileInfo(include).isAbsolute()
                ? QDir::cleanPath(include)
                : QDir(QFileInfo(path).absolutePath()).absoluteFilePath(include);
            unit.sources.push_back(absolute.toStdString());
        }
        QStringList command;
        for (const auto& argument : entry.value(QStringLiteral("arguments")).toArray()) {
            command.push_back(argument.toString());
        }
        if (command.isEmpty()) {
            command = QProcess::splitCommand(entry.value(QStringLiteral("command")).toString());
        }
        if (command.isEmpty() || unit.sources.empty()) {
            continue;
        }
        unit.compiler = command.takeFirst();
        unit.arguments = command;
        units.push_back(std::move(unit));
    }
    return units;
}

QString quotedIncludePath(const std::string& source) {
    QString path = QString::fromStdString(source);
    path.replace('\\', QStringLiteral("\\\\"));
    path.replace('"', QStringLiteral("\\\""));
    return path;
}

ProcessResult runProbe(
    const ProcessRunner& runner,
    const doctor::domain::ProjectConfig& config,
    const UnityCompileUnit& unit,
    const std::vector<std::string>& sources,
    const QString& probeRoot,
    int probeNumber,
    std::atomic_bool& cancelled,
    const doctor::application::LogSink& sink) {
    QDir().mkpath(probeRoot);
    const auto suffix = QFileInfo(unit.source).suffix();
    const auto driver = QDir(probeRoot).filePath(
        QStringLiteral("probe-%1.%2").arg(probeNumber).arg(suffix));
    QFile driverFile(driver);
    if (!driverFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        ProcessResult failed;
        failed.output = driverFile.errorString();
        return failed;
    }
    for (const auto& source : sources) {
        driverFile.write(
            QStringLiteral("#include \"%1\"\n").arg(quotedIncludePath(source)).toUtf8());
    }
    driverFile.close();

    const auto output = QDir(probeRoot).filePath(
        QStringLiteral("probe-%1.o").arg(probeNumber));
    QStringList arguments;
    bool replacedSource = false;
    for (int index = 0; index < unit.arguments.size(); ++index) {
        const auto& argument = unit.arguments[index];
        if (argument == unit.source) {
            arguments.push_back(driver);
            replacedSource = true;
            continue;
        }
        if ((argument == QStringLiteral("-o") ||
             argument == QStringLiteral("-MF") ||
             argument == QStringLiteral("-MT") ||
             argument == QStringLiteral("-MQ")) &&
            index + 1 < unit.arguments.size()) {
            if (argument == QStringLiteral("-o")) {
                arguments << argument << output;
            }
            ++index;
            continue;
        }
        if (argument.startsWith(QStringLiteral("-o")) && argument.size() > 2) {
            arguments.push_back(QStringLiteral("-o") + output);
            continue;
        }
        arguments.push_back(argument);
    }
    if (!replacedSource) {
        arguments.push_back(driver);
    }
    return runner.run(
        unit.compiler,
        arguments,
        unit.directory,
        config.timeoutSeconds,
        cancelled,
        [&](const QString& text) { sink(text.toStdString()); });
}

}  // namespace

doctor::application::ProjectInventory CMakeBackend::inspect(
    const doctor::domain::ProjectConfig& config,
    std::atomic_bool& cancelled,
    const doctor::application::LogSink& log) {
    doctor::application::ProjectInventory inventory;
    const auto root = QString::fromStdString(config.workDirectory);
    inventory.baselineBuildDirectory =
        QDir(root).filePath(QStringLiteral("baseline")).toStdString();
    inventory.unityBuildDirectory =
        QDir(root).filePath(QStringLiteral("unity")).toStdString();
    const auto baseline = configure(
        runner_, config, QString::fromStdString(inventory.baselineBuildDirectory),
        false, cancelled, log);
    if (baseline.exitCode != 0 || cancelled.load()) {
        inventory.error = cancelled.load() ? "分析已取消" : "普通构建配置失败";
        return inventory;
    }
    const auto unity = configure(
        runner_, config, QString::fromStdString(inventory.unityBuildDirectory),
        true, cancelled, log);
    if (unity.exitCode != 0 || cancelled.load()) {
        inventory.error = cancelled.load() ? "分析已取消" : "Unity 构建配置失败";
        return inventory;
    }
    inventory.targets = readTargets(
        QString::fromStdString(inventory.baselineBuildDirectory));
    if (!config.targetFilter.empty()) {
        inventory.targets.erase(
            std::remove_if(
                inventory.targets.begin(), inventory.targets.end(),
                [&](const auto& target) {
                    return std::find(
                        config.targetFilter.begin(), config.targetFilter.end(),
                        target.name) == config.targetFilter.end();
                }),
            inventory.targets.end());
    }
    inventory.valid = !inventory.targets.empty();
    if (!inventory.valid) {
        inventory.error = "CMake File API 未发现可构建 C/C++ target";
    }
    return inventory;
}

doctor::domain::TargetResult CMakeBackend::analyze(
    const doctor::domain::ProjectConfig& config,
    const doctor::application::ProjectInventory& inventory,
    const doctor::domain::TargetResult& pending,
    std::atomic_bool& cancelled,
    const doctor::application::LogSink& log) {
    auto result = pending;
    result.status = doctor::domain::TargetStatus::Running;
    result.stage = "baseline";
    const auto baseline = buildTarget(
        runner_, config, inventory.baselineBuildDirectory, pending.name,
        cancelled, log);
    if (cancelled.load()) {
        result.status = doctor::domain::TargetStatus::Cancelled;
        return result;
    }
    if (baseline.exitCode != 0) {
        result.status = doctor::domain::TargetStatus::BaselineFailed;
        result.stage = baseline.timedOut ? "timeout" : "baseline";
        result.logPath = config.workDirectory + "/analysis.log";
        return result;
    }
    result.stage = "unity";
    const auto unity = buildTarget(
        runner_, config, inventory.unityBuildDirectory, pending.name,
        cancelled, log);
    if (cancelled.load()) {
        result.status = doctor::domain::TargetStatus::Cancelled;
        return result;
    }
    if (unity.exitCode == 0) {
        result.status = doctor::domain::TargetStatus::Passed;
        return result;
    }
    result.status = doctor::domain::TargetStatus::UnityFailed;
    result.logPath = config.workDirectory + "/analysis.log";
    result.stage = unity.timedOut ? "timeout" : "compile";
    const auto failure = fingerprint(unity.output);
    const auto units = unityCompileUnits(
        QString::fromStdString(inventory.unityBuildDirectory),
        QString::fromStdString(pending.name));
    std::vector<std::string> sources;
    doctor::domain::MinimizationResult minimized;
    bool matchedUnit = false;
    int probeNumber = 0;
    const auto probeRoot = QDir(QString::fromStdString(config.workDirectory)).filePath(
        QStringLiteral("probes/%1").arg(QString::fromStdString(pending.name)));
    for (const auto& unit : units) {
        const auto fullProbe = runProbe(
            runner_, config, unit, unit.sources, probeRoot, ++probeNumber,
            cancelled, log);
        if (cancelled.load()) {
            result.status = doctor::domain::TargetStatus::Cancelled;
            return result;
        }
        if (fullProbe.exitCode == 0 ||
            fingerprint(fullProbe.output).key() != failure.key()) {
            continue;
        }
        matchedUnit = true;
        minimized = doctor::domain::minimizeOrdered(
            unit.sources,
            [&](const std::vector<std::string>& candidate) {
                const auto probe = runProbe(
                    runner_, config, unit, candidate, probeRoot, ++probeNumber,
                    cancelled, log);
                return probe.exitCode != 0 &&
                    fingerprint(probe.output).key() == failure.key();
            },
            config.maxProbes);
        sources = minimized.sources;
        break;
    }
    if (!matchedUnit) {
        result.status = doctor::domain::TargetStatus::NonReplayable;
        result.stage = "probe";
        if (!units.empty()) {
            sources = units.front().sources;
        }
    } else if (minimized.status ==
               doctor::domain::MinimizationResult::Status::NonReplayable) {
        result.status = doctor::domain::TargetStatus::NonReplayable;
        result.stage = "probe";
    } else if (minimized.status ==
               doctor::domain::MinimizationResult::Status::BudgetExhausted) {
        result.stage = "probe-budget";
    } else {
        result.stage = minimized.orderSensitive ? "order-sensitive" : "minimized";
    }
    std::map<std::string, std::string> texts;
    for (const auto& source : sources) {
        QFile file(QString::fromStdString(source));
        if (file.open(QIODevice::ReadOnly)) {
            texts[source] = file.readAll().toStdString();
        }
    }
    result.issues = doctor::domain::buildIssues(
        pending.name, failure, sources, texts, {3, 27, 1});
    if (!result.issues.empty()) {
        result.issues.front().evidence.push_back(
            "probe_count=" + std::to_string(probeNumber));
        if (result.status == doctor::domain::TargetStatus::NonReplayable) {
            result.issues.front().confidence =
                std::min(result.issues.front().confidence, 0.45);
            result.issues.front().suggestion =
                "Unity 失败已确认，但编译命令无法稳定复放；请先核对完整日志和工具链环境。";
        }
    }
    return result;
}

}  // namespace doctor::infrastructure
