#pragma once

#include "doctor/domain/source_scan.h"

#include <string>
#include <vector>

namespace doctor::domain::source_scan_internal {

struct Definition {
    std::string name;
    std::string value;
    std::string path;
    std::string evidence;
    int line{0};
};

struct FileFacts {
    std::vector<Definition> activeMacros;
    std::vector<Definition> usingNamespaces;
    std::vector<Definition> staticSymbols;
};

FileFacts analyzeDocument(const SourceDocument& document);
std::string skipSnippet(const std::vector<std::string>& sources);

}  // namespace doctor::domain::source_scan_internal
