#pragma once

#include "doctor/domain/models.h"

#include <string>
#include <vector>

namespace doctor::domain {

struct SourceDocument {
    std::string path;
    std::string text;
};

std::vector<Issue> scanSourceRisks(
    const std::vector<SourceDocument>& documents);

}  // namespace doctor::domain
