#include "doctor/domain/models.h"

namespace doctor::domain {

std::string toString(AnalysisMode mode) {
    switch (mode) {
    case AnalysisMode::BuildVerification: return "build-verification";
    case AnalysisMode::SourceScan: return "source-scan";
    }
    return "build-verification";
}

std::string FailureFingerprint::key() const {
    return compilerFamily + "|" + phase + "|" + category + "|" + symbol + "|" + message;
}

std::string toString(TargetStatus status) {
    switch (status) {
    case TargetStatus::Pending: return "Pending";
    case TargetStatus::Running: return "Running";
    case TargetStatus::Passed: return "Passed";
    case TargetStatus::BaselineFailed: return "Baseline Failed";
    case TargetStatus::UnityFailed: return "Unity Failed";
    case TargetStatus::RiskFound: return "Risk Found";
    case TargetStatus::NonReplayable: return "Non-replayable";
    case TargetStatus::Cancelled: return "Cancelled";
    case TargetStatus::Unsupported: return "Unsupported";
    }
    return "Unknown";
}

}  // namespace doctor::domain
