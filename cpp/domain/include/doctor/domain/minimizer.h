#pragma once

#include <functional>
#include <string>
#include <vector>

namespace doctor::domain {

struct MinimizationResult {
    enum class Status { Minimized, NonReplayable, BudgetExhausted };
    Status status{Status::NonReplayable};
    std::vector<std::string> sources;
    int probes{0};
    bool orderSensitive{false};
};

using ProbePredicate = std::function<bool(const std::vector<std::string>&)>;

MinimizationResult minimizeOrdered(
    const std::vector<std::string>& candidates,
    const ProbePredicate& reproduces,
    int maxProbes);

}  // namespace doctor::domain
