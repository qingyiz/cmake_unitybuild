#include "doctor/domain/models.h"

namespace doctor::domain {

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
    case TargetStatus::NonReplayable: return "Non-replayable";
    case TargetStatus::Cancelled: return "Cancelled";
    case TargetStatus::Unsupported: return "Unsupported";
    }
    return "Unknown";
}

}  // namespace doctor::domain
