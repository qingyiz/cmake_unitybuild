#pragma once

#include "doctor/domain/models.h"

#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace doctor::domain {

Classification classify(
    const FailureFingerprint& fingerprint,
    const std::vector<std::string>& orderedSources,
    const std::map<std::string, std::string>& sourceTexts,
    const std::map<std::string, std::string>& compileSignatures = {});

std::vector<Issue> buildIssues(
    const std::string& target,
    const FailureFingerprint& fingerprint,
    const std::vector<std::string>& minimalSources,
    const std::map<std::string, std::string>& sourceTexts,
    const std::tuple<int, int, int>& cmakeVersion);

}  // namespace doctor::domain
