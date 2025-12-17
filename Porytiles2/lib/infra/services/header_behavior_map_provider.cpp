#include "porytiles2/infra/services/header_behavior_map_provider.hpp"

#include <limits>
#include <utility>

#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/text/file_highlight_printer.hpp"

namespace porytiles2 {

namespace {

/**
 * @brief Builds a multi-line FormattableError showing both duplicate locations with source context.
 */
[[nodiscard]] FormattableError make_duplicate_error(
    const std::string &header_message,
    SourcePosition duplicate_pos,
    const std::string &note_message,
    SourcePosition original_pos,
    const std::vector<std::string> &file_lines,
    const TextFormatter *format)
{
    std::vector<std::string> lines;
    FileHighlightPrinter printer{format};

    // Add header for duplicate location
    lines.push_back(header_message);

    // Add source context for duplicate location (convert 1-based to 0-based)
    assert_or_panic(duplicate_pos.line > 0, "duplicate_pos.line must be positive (1-based)");
    assert_or_panic(duplicate_pos.line <= file_lines.size(), "duplicate_pos.line exceeds file bounds");
    assert_or_panic(duplicate_pos.column > 0, "duplicate_pos.column must be positive (1-based)");
    auto dup_context = printer.print(file_lines, duplicate_pos.line - 1, duplicate_pos.column - 1);
    for (auto &line : dup_context) {
        lines.push_back(std::move(line));
    }

    // Add blank line separator
    lines.emplace_back("");

    // Add note about original location
    lines.push_back(note_message);

    // Add source context for original location (convert 1-based to 0-based)
    assert_or_panic(original_pos.line > 0, "original_pos.line must be positive (1-based)");
    assert_or_panic(original_pos.line <= file_lines.size(), "original_pos.line exceeds file bounds");
    assert_or_panic(original_pos.column > 0, "original_pos.column must be positive (1-based)");
    auto orig_context = printer.print(file_lines, original_pos.line - 1, original_pos.column - 1);
    for (auto &line : orig_context) {
        lines.push_back(std::move(line));
    }

    return FormattableError{std::move(lines)};
}

} // namespace

template <typename Entry>
ChainableResult<void> HeaderBehaviorMapProvider::try_add_behavior_entry(const Entry &entry) const
{
    const auto &name = entry.name();

    // Filter: must start with MB_
    if (!name.starts_with("MB_")) {
        return {};
    }

    auto raw_value = entry.int_value();

    // Filter: value must be in valid range
    if (raw_value < 0 || raw_value > std::numeric_limits<std::uint16_t>::max()) {
        return {};
    }

    auto value = static_cast<std::uint16_t>(raw_value);
    const auto &new_pos = entry.position();
    const auto header_path = header_relative_path_.string();

    // Check for duplicate name
    if (name_to_value_.contains(name)) {
        load_failed_ = true;
        const auto &orig_pos = name_to_position_.at(name);
        return make_duplicate_error(
            format_->format(
                "{}:{}:{}: duplicate behavior name '{}'",
                FormatParam{header_path, Style::bold},
                new_pos.line,
                new_pos.column,
                FormatParam{name, Style::bold}),
            new_pos,
            format_->format(
                "{} originally defined at line {}:", FormatParam{"note:", Style::cyan | Style::bold}, orig_pos.line),
            orig_pos,
            driver_->file_lines(),
            format_);
    }

    // Check for duplicate value
    if (value_to_name_.contains(value)) {
        load_failed_ = true;
        const auto &orig_name = value_to_name_.at(value);
        const auto &orig_pos = value_to_position_.at(value);
        return make_duplicate_error(
            format_->format(
                "{}:{}:{}: duplicate behavior value '{}': both '{}' and '{}' have this value",
                FormatParam{header_path, Style::bold},
                new_pos.line,
                new_pos.column,
                FormatParam{value, Style::bold},
                FormatParam{orig_name, Style::bold},
                FormatParam{name, Style::bold}),
            new_pos,
            format_->format(
                "{} '{}' originally defined at line {}:",
                FormatParam{"note:", Style::cyan | Style::bold},
                FormatParam{orig_name, Style::bold},
                orig_pos.line),
            orig_pos,
            driver_->file_lines(),
            format_);
    }

    // Insert into all maps
    name_to_value_[name] = value;
    value_to_name_[value] = name;
    name_to_position_[name] = new_pos;
    value_to_position_[value] = new_pos;

    return {};
}

ChainableResult<std::uint16_t> HeaderBehaviorMapProvider::lookup(const std::string &behavior_name) const
{
    if (!behavior_name.starts_with("MB_")) {
        return FormattableError{
            "invalid behavior name '{}': expected prefix '{}'",
            FormatParam{behavior_name, Style::bold},
            FormatParam{"MB_", Style::bold}};
    }

    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::uint16_t>{
            FormattableError{"metatile behavior provider lookup failed"}, load_result};
    }

    const auto it = name_to_value_.find(behavior_name);
    if (it == name_to_value_.end()) {
        return FormattableError{
            "behavior '{}' not found in '{}'",
            FormatParam{behavior_name, Style::bold},
            FormatParam{header_relative_path_.string(), Style::bold}};
    }
    return it->second;
}

ChainableResult<std::string> HeaderBehaviorMapProvider::lookup(std::uint16_t behavior_value) const
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::string>{FormattableError{"metatile behavior provider lookup failed"}, load_result};
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

    // Create and store CParserDriver for rich error formatting with source context
    driver_ = std::make_unique<CParserDriver>(header_path, format_);

    // Parse #define statements
    auto defines_result = driver_->parse_defines();
    if (!defines_result.has_value()) {
        load_failed_ = true;
        return ChainableResult<void>{defines_result};
    }
    for (const auto &def : defines_result.value()) {
        if (!def.has_int_value()) {
            continue;
        }
        auto insert_result = try_add_behavior_entry(def);
        if (!insert_result.has_value()) {
            return insert_result;
        }
    }

    // Parse enum declarations
    auto enums_result = driver_->parse_enums();
    if (!enums_result.has_value()) {
        load_failed_ = true;
        return ChainableResult<void>{enums_result};
    }
    for (const auto &enum_decl : enums_result.value()) {
        for (const auto &member : enum_decl.members()) {
            auto insert_result = try_add_behavior_entry(member);
            if (!insert_result.has_value()) {
                return insert_result;
            }
        }
    }

    if (name_to_value_.empty()) {
        load_failed_ = true;
        return FormattableError{
            "{}: no behavior definitions exist in file", FormatParam{header_path.string(), Style::bold}};
    }

    return {};
}

} // namespace porytiles2
