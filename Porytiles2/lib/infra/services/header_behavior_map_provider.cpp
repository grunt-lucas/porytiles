#include "porytiles2/infra/services/header_behavior_map_provider.hpp"

#include <fstream>
#include <limits>
#include <sstream>
#include <string>

#include "porytiles2/utilities/c_parser/lexer.hpp"
#include "porytiles2/utilities/c_parser/parser.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace porytiles2 {

HeaderBehaviorMapProvider::HeaderBehaviorMapProvider(
    const std::filesystem::path &project_root,
    const std::filesystem::path &header_relative_path,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag)
    : project_root_{project_root}, header_relative_path_{header_relative_path}, format_{format}, diag_{diag},
      file_printer_{std::make_unique<FileHighlightPrinter>(format)}
{
}

ChainableResult<std::uint16_t> HeaderBehaviorMapProvider::lookup(const std::string &behavior_name) const
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::uint16_t>{FormattableError{"behavior lookup failed"}, load_result};
    }

    const auto it = name_to_value_.find(behavior_name);
    if (it == name_to_value_.end()) {
        return FormattableError{
            "unknown behavior '{}' not found in '{}'",
            FormatParam{behavior_name, Style::bold},
            FormatParam{header_relative_path_.string(), Style::bold}};
    }
    return it->second;
}

ChainableResult<std::string> HeaderBehaviorMapProvider::lookup(std::uint16_t behavior_value) const
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::string>{FormattableError{"behavior lookup failed"}, load_result};
    }

    const auto it = value_to_name_.find(behavior_value);
    if (it == value_to_name_.end()) {
        return FormattableError{
            "unknown behavior value '{}' not found in '{}'",
            FormatParam{behavior_value, Style::bold},
            FormatParam{header_relative_path_.string(), Style::bold}};
    }
    return it->second;
}

ChainableResult<void> HeaderBehaviorMapProvider::ensure_loaded() const
{
    if (loaded_) {
        if (load_failed_) {
            return FormattableError{"behavior header file previously failed to load"};
        }
        return {};
    }

    loaded_ = true;

    const auto header_path = project_root_ / header_relative_path_;

    if (!std::filesystem::exists(header_path)) {
        load_failed_ = true;
        std::vector<std::string> err_lines{};
        err_lines.push_back(
            format_->format("{}: behavior header file does not exist", FormatParam{header_path.string(), Style::bold}));
        diag_->err("behavior-header-load-failure", err_lines);
        return FormattableError{"behavior header file not found"};
    }

    std::ifstream file{header_path};
    if (!file.is_open()) {
        load_failed_ = true;
        std::vector<std::string> err_lines{};
        err_lines.push_back(
            format_->format("{}: failed to open behavior header file", FormatParam{header_path.string(), Style::bold}));
        diag_->err("behavior-header-load-failure", err_lines);
        return FormattableError{"failed to open behavior header file"};
    }

    // Read entire file content for parser, and cache lines for FileHighlightPrinter
    std::ostringstream content_stream;
    std::string line;
    while (std::getline(file, line)) {
        trim_line_ending(line);
        cached_lines_.push_back(line);
        content_stream << line << '\n';
    }
    std::string content = content_stream.str();

    // Tokenize and parse
    Lexer lexer{content};
    auto tokens_result = lexer.lex();
    if (!tokens_result.has_value()) {
        load_failed_ = true;
        std::vector<std::string> err_lines{};
        err_lines.push_back(format_->format(
            "{}: failed to tokenize behavior header file", FormatParam{header_path.string(), Style::bold}));
        diag_->err("behavior-header-load-failure", err_lines);
        return ChainableResult<void>{FormattableError{"failed to tokenize behavior header file"}, tokens_result};
    }

    Parser parser{std::move(tokens_result).value()};

    // Parse #define statements
    auto defines_result = parser.parse_defines();
    if (defines_result.has_value()) {
        for (const auto &def : defines_result.value()) {
            if (!def.has_int_value()) {
                continue;
            }
            const auto &name = def.name();

            // Filter: must start with MB_ and not be MB_INVALID
            if (!name.starts_with("MB_") || name == "MB_INVALID") {
                continue;
            }

            auto value = def.int_value();
            if (value < 0 || value > std::numeric_limits<std::uint16_t>::max()) {
                continue;
            }

            auto uint_value = static_cast<std::uint16_t>(value);
            name_to_value_[name] = uint_value;
            if (!value_to_name_.contains(uint_value)) {
                value_to_name_[uint_value] = name;
            }
        }
    }

    // Parse enum declarations
    auto enums_result = parser.parse_enums();
    if (enums_result.has_value()) {
        for (const auto &enum_decl : enums_result.value()) {
            for (const auto &member : enum_decl.members()) {
                const auto &name = member.name();

                // Filter: must start with MB_ and not be MB_INVALID
                if (!name.starts_with("MB_") || name == "MB_INVALID") {
                    continue;
                }

                auto value = member.value();
                if (value < 0 || value > std::numeric_limits<std::uint16_t>::max()) {
                    continue;
                }

                auto uint_value = static_cast<std::uint16_t>(value);

                // Don't overwrite existing entries (defines take precedence)
                if (!name_to_value_.contains(name)) {
                    name_to_value_[name] = uint_value;
                }
                if (!value_to_name_.contains(uint_value)) {
                    value_to_name_[uint_value] = name;
                }
            }
        }
    }

    if (name_to_value_.empty()) {
        load_failed_ = true;
        std::vector<std::string> err_lines{};
        err_lines.push_back(format_->format(
            "{}: no behavior definitions found (expected MB_* defines or enum entries)",
            FormatParam{header_path.string(), Style::bold}));
        diag_->err("behavior-header-load-failure", err_lines);
        return FormattableError{"no behavior definitions found in header file"};
    }

    return {};
}

} // namespace porytiles2
