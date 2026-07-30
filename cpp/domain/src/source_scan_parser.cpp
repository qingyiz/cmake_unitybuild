#include "source_scan_internal.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <regex>
#include <sstream>

namespace doctor::domain::source_scan_internal {
namespace {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string normalized(const std::string& value) {
    std::string result;
    bool spacing = false;
    for (const auto character : trim(value)) {
        if (std::isspace(static_cast<unsigned char>(character))) {
            spacing = !result.empty();
        } else {
            if (spacing) {
                result.push_back(' ');
            }
            result.push_back(character);
            spacing = false;
        }
    }
    return result;
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::string stripCommentsAndLiterals(const std::string& text) {
    enum class State { Normal, LineComment, BlockComment, String, Character };
    State state = State::Normal;
    bool escaped = false;
    std::string result(text.size(), ' ');
    for (std::size_t index = 0; index < text.size(); ++index) {
        const auto character = text[index];
        const auto next = index + 1 < text.size() ? text[index + 1] : '\0';
        if (character == '\n') {
            result[index] = '\n';
            if (state == State::LineComment) {
                state = State::Normal;
            }
            escaped = false;
            continue;
        }
        if (state == State::Normal) {
            if (character == '/' && next == '/') {
                state = State::LineComment;
                ++index;
            } else if (character == '/' && next == '*') {
                state = State::BlockComment;
                ++index;
            } else if (character == '"') {
                state = State::String;
            } else if (character == '\'') {
                state = State::Character;
            } else {
                result[index] = character;
            }
            continue;
        }
        if (state == State::BlockComment && character == '*' && next == '/') {
            state = State::Normal;
            ++index;
            continue;
        }
        if (state == State::String || state == State::Character) {
            if (escaped) {
                escaped = false;
            } else if (character == '\\') {
                escaped = true;
            } else if ((state == State::String && character == '"') ||
                       (state == State::Character && character == '\'')) {
                state = State::Normal;
            }
        }
    }
    return result;
}

std::string lastIdentifier(const std::string& value) {
    static const std::regex identifier(R"(\b([A-Za-z_][A-Za-z0-9_]*)\b)");
    std::string result;
    for (auto iterator = std::sregex_iterator(
             value.begin(), value.end(), identifier);
         iterator != std::sregex_iterator();
         ++iterator) {
        result = (*iterator)[1].str();
    }
    return result;
}

void collectStatic(
    const std::string& statement,
    const std::string& original,
    const std::string& path,
    int line,
    std::vector<Definition>* definitions) {
    static const std::regex storage(R"(\bstatic\b)");
    if (!std::regex_search(statement, storage) ||
        statement.find("static_assert") != std::string::npos ||
        statement.find("static_cast") != std::string::npos) {
        return;
    }
    auto boundary = statement.find(';');
    for (const auto marker : {'=', '(', '['}) {
        const auto position = statement.find(marker);
        if (position != std::string::npos) {
            boundary = std::min(boundary, position);
        }
    }
    const auto prefix = statement.substr(
        0, boundary == std::string::npos ? statement.size() : boundary);
    const auto name = lastIdentifier(prefix);
    if (!name.empty() && name != "static") {
        definitions->push_back(
            {name, {}, path,
             path + ":" + std::to_string(line) + ": " + normalized(original),
             line});
    }
}

}  // namespace

FileFacts analyzeDocument(const SourceDocument& document) {
    FileFacts facts;
    const auto originalLines = splitLines(document.text);
    const auto cleanLines = splitLines(stripCommentsAndLiterals(document.text));
    std::map<std::string, Definition> activeMacros;
    static const std::regex directive(
        R"(^\s*#\s*(define|undef)\s+([A-Za-z_][A-Za-z0-9_]*)(.*)$)");
    for (std::size_t index = 0; index < originalLines.size(); ++index) {
        std::smatch match;
        if (index >= cleanLines.size() ||
            !std::regex_match(cleanLines[index], match, directive)) {
            continue;
        }
        const auto name = match[2].str();
        if (match[1].str() == "undef") {
            activeMacros.erase(name);
        } else {
            std::smatch originalMatch;
            std::regex_match(originalLines[index], originalMatch, directive);
            activeMacros[name] = {
                name, normalized(originalMatch[3].str()), document.path,
                document.path + ":" + std::to_string(index + 1) + ": " +
                    normalized(originalLines[index]),
                static_cast<int>(index + 1)};
        }
    }
    for (const auto& item : activeMacros) {
        facts.activeMacros.push_back(item.second);
    }

    static const std::regex usingPattern(
        R"(^\s*using\s+namespace\s+([A-Za-z_][A-Za-z0-9_:]*)\s*;)");
    int depth = 0;
    std::string statement;
    std::string originalStatement;
    int statementLine = 0;
    for (std::size_t index = 0; index < cleanLines.size(); ++index) {
        const auto& cleanLine = cleanLines[index];
        const auto originalLine =
            index < originalLines.size() ? originalLines[index] : cleanLine;
        if (trim(cleanLine).rfind("#", 0) == 0) {
            continue;
        }
        if (depth == 0) {
            std::smatch match;
            if (std::regex_search(cleanLine, match, usingPattern)) {
                facts.usingNamespaces.push_back({
                    match[1].str(), {}, document.path,
                    document.path + ":" + std::to_string(index + 1) + ": " +
                        normalized(originalLine),
                    static_cast<int>(index + 1)});
            }
        }
        for (std::size_t column = 0; column < cleanLine.size(); ++column) {
            const auto character = cleanLine[column];
            if (depth == 0 && statementLine == 0 &&
                !std::isspace(static_cast<unsigned char>(character))) {
                statementLine = static_cast<int>(index + 1);
            }
            if (depth == 0 && character != '}') {
                statement.push_back(character);
                originalStatement.push_back(
                    column < originalLine.size() ? originalLine[column] : character);
            }
            if (character == '{') {
                if (depth == 0) {
                    collectStatic(
                        statement, originalStatement, document.path,
                        statementLine, &facts.staticSymbols);
                    statement.clear();
                    originalStatement.clear();
                    statementLine = 0;
                }
                ++depth;
            } else if (character == '}') {
                depth = std::max(0, depth - 1);
                if (depth == 0) {
                    statement.clear();
                    originalStatement.clear();
                    statementLine = 0;
                }
            } else if (character == ';' && depth == 0) {
                collectStatic(
                    statement, originalStatement, document.path,
                    statementLine, &facts.staticSymbols);
                statement.clear();
                originalStatement.clear();
                statementLine = 0;
            }
        }
        if (depth == 0 && !statement.empty()) {
            statement.push_back('\n');
            originalStatement.push_back('\n');
        }
    }
    return facts;
}

std::string skipSnippet(const std::vector<std::string>& sources) {
    std::ostringstream output;
    output << "set_source_files_properties(\n";
    for (const auto& source : sources) {
        output << "    \"" << source << "\"\n";
    }
    output << "    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON\n)";
    return output.str();
}

}  // namespace doctor::domain::source_scan_internal
