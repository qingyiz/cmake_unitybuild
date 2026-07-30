#pragma once

#include <string>
#include <vector>

namespace doctor::domain {

enum class AnalysisMode {
    BuildVerification,
    SourceScan
};

enum class TargetStatus {
    Pending,
    Running,
    Passed,
    BaselineFailed,
    UnityFailed,
    RiskFound,
    NonReplayable,
    Cancelled,
    Unsupported
};

struct ProjectConfig {
    AnalysisMode analysisMode{AnalysisMode::BuildVerification};
    std::string sourceDirectory;
    std::string workDirectory;
    std::string cmakeExecutable{"cmake"};
    std::string generator{"Ninja"};
    std::string configuration{"Debug"};
    std::vector<std::string> cmakeArguments;
    std::vector<std::string> targetFilter;
    int parallelJobs{0};
    int maxProbes{100};
    int timeoutSeconds{300};
};

struct Issue {
    std::string id;
    std::string ruleId;
    std::string target;
    std::string category;
    std::string severity{"error"};
    std::string summary;
    std::string fingerprint;
    std::vector<std::string> sources;
    std::vector<std::string> evidence;
    std::string suggestion;
    std::string cmakeSnippet;
    double confidence{0.0};
};

struct FailureFingerprint {
    std::string compilerFamily;
    std::string phase{"compile"};
    std::string category;
    std::string symbol;
    std::string message;

    std::string key() const;
};

struct Classification {
    std::string category{"UNKNOWN"};
    std::string summary;
    std::vector<std::string> evidence;
    std::vector<std::string> alternatives;
    double confidence{0.0};
};

struct TargetResult {
    std::string name;
    std::string type;
    TargetStatus status{TargetStatus::Pending};
    std::string stage;
    std::string logPath;
    std::vector<Issue> issues;
};

struct ProjectSession {
    std::string analysisMode{"build-verification"};
    std::string sourceDirectory;
    std::string workDirectory;
    std::vector<TargetResult> targets;
    bool cancelled{false};
};

std::string toString(TargetStatus status);
std::string toString(AnalysisMode mode);

}  // namespace doctor::domain
