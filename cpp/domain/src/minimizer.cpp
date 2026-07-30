#include "doctor/domain/minimizer.h"

#include <algorithm>

namespace doctor::domain {

MinimizationResult minimizeOrdered(
    const std::vector<std::string>& candidates,
    const ProbePredicate& predicate,
    int maxProbes) {
    MinimizationResult result;
    result.sources = candidates;
    auto probe = [&](const std::vector<std::string>& sources) -> bool {
        if (result.probes >= maxProbes) {
            return false;
        }
        ++result.probes;
        return predicate(sources);
    };
    if (candidates.empty() || !probe(candidates)) {
        result.status = result.probes >= maxProbes
            ? MinimizationResult::Status::BudgetExhausted
            : MinimizationResult::Status::NonReplayable;
        return result;
    }

    std::size_t granularity = 2;
    while (result.sources.size() >= 2 && result.probes < maxProbes) {
        const auto chunkSize = (result.sources.size() + granularity - 1) / granularity;
        bool reduced = false;
        for (std::size_t begin = 0; begin < result.sources.size(); begin += chunkSize) {
            const auto end = std::min(result.sources.size(), begin + chunkSize);
            std::vector<std::string> part(
                result.sources.begin() + static_cast<long>(begin),
                result.sources.begin() + static_cast<long>(end));
            if (probe(part)) {
                result.sources = std::move(part);
                granularity = std::max<std::size_t>(2, granularity - 1);
                reduced = true;
                break;
            }
        }
        if (reduced) {
            continue;
        }
        for (std::size_t begin = 0; begin < result.sources.size(); begin += chunkSize) {
            const auto end = std::min(result.sources.size(), begin + chunkSize);
            std::vector<std::string> complement;
            for (std::size_t index = 0; index < result.sources.size(); ++index) {
                if (index < begin || index >= end) {
                    complement.push_back(result.sources[index]);
                }
            }
            if (!complement.empty() && probe(complement)) {
                result.sources = std::move(complement);
                granularity = std::max<std::size_t>(2, granularity - 1);
                reduced = true;
                break;
            }
        }
        if (reduced) {
            continue;
        }
        if (granularity >= result.sources.size()) {
            break;
        }
        granularity = std::min(result.sources.size(), granularity * 2);
    }

    for (std::size_t index = 0;
         index < result.sources.size() && result.probes < maxProbes;
         ++index) {
        auto without = result.sources;
        without.erase(without.begin() + static_cast<long>(index));
        if (!without.empty() && probe(without)) {
            result.sources = std::move(without);
            index = static_cast<std::size_t>(-1);
        }
    }
    if (result.probes >= maxProbes) {
        result.status = MinimizationResult::Status::BudgetExhausted;
        return result;
    }
    auto reversed = result.sources;
    std::reverse(reversed.begin(), reversed.end());
    result.orderSensitive = reversed != result.sources && !probe(reversed);
    result.status = MinimizationResult::Status::Minimized;
    return result;
}

}  // namespace doctor::domain
