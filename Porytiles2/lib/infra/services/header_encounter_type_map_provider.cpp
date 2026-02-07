#include "porytiles2/infra/services/header_encounter_type_map_provider.hpp"

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

    lines.push_back(header_message);

    assert_or_panic(duplicate_pos.line > 0, "duplicate_pos.line must be positive (1-based)");
    assert_or_panic(duplicate_pos.line <= file_lines.size(), "duplicate_pos.line exceeds file bounds");
    assert_or_panic(duplicate_pos.column > 0, "duplicate_pos.column must be positive (1-based)");
    auto dup_context = printer.print(file_lines, duplicate_pos.line - 1, duplicate_pos.column - 1);
    for (auto &line : dup_context) {
        lines.push_back(std::move(line));
    }

    lines.emplace_back("");

    lines.push_back(note_message);

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
ChainableResult<void> HeaderEncounterTypeMapProvider::try_add_encounter_entry(const Entry &entry) const
{
    const auto &name = entry.name();

    // Filter: must start with TILE_ENCOUNTER_
    if (!name.starts_with("TILE_ENCOUNTER_")) {
        return {};
    }

    auto raw_value = entry.int_value();

    // Filter: value must be in valid range (3 bits: 0-7)
    if (raw_value < 0 || raw_value > 7) {
        return {};
    }

    auto value = static_cast<std::uint8_t>(raw_value);
    const auto &new_pos = entry.position();

    // Check for duplicate name
    if (name_to_value_.contains(name)) {
        load_failed_ = true;
        const auto &orig_pos = name_to_position_.at(name);
        return make_duplicate_error(
            format_->format(
                "{}:{}:{}: duplicate encounter type name '{}'.",
                FormatParam{header_path_, Style::bold},
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
                "{}:{}:{}: duplicate encounter type value '{}': both '{}' and '{}' have this value.",
                FormatParam{header_path_, Style::bold},
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

ChainableResult<std::uint8_t> HeaderEncounterTypeMapProvider::lookup(const std::string &encounter_name) const
{
    if (!encounter_name.starts_with("TILE_ENCOUNTER_")) {
        return FormattableError{
            "Invalid encounter type name '{}': expected prefix '{}'.",
            FormatParam{encounter_name, Style::bold},
            FormatParam{"TILE_ENCOUNTER_", Style::bold}};
    }

    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::uint8_t>{FormattableError{"Encounter type provider lookup failed."}, load_result};
    }

    const auto it = name_to_value_.find(encounter_name);
    if (it == name_to_value_.end()) {
        return FormattableError{
            "Encounter type '{}' not found in '{}'.",
            FormatParam{encounter_name, Style::bold},
            FormatParam{header_path_.string(), Style::bold}};
    }
    return it->second;
}

ChainableResult<std::string> HeaderEncounterTypeMapProvider::lookup(std::uint8_t encounter_value) const
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::string>{FormattableError{"Encounter type provider lookup failed."}, load_result};
    }

    const auto it = value_to_name_.find(encounter_value);
    if (it == value_to_name_.end()) {
        return FormattableError{
            "Unknown encounter type value '{}' not found in '{}'.",
            FormatParam{encounter_value, Style::bold},
            FormatParam{header_path_.string(), Style::bold}};
    }
    return it->second;
}

ChainableResult<void> HeaderEncounterTypeMapProvider::ensure_loaded() const
{
    if (loaded_) {
        if (load_failed_) {
            return FormattableError{"Encounter type header file previously failed to load."};
        }
        return {};
    }

    loaded_ = true;

    // Create and store CParserFacade for rich error formatting with source context
    driver_ = std::make_unique<CParserFacade>(header_path_, format_);

    // Only parse enum declarations — encounter type constants are defined as enum members in global.fieldmap.h.
    // We intentionally skip parse_defines() because the header may contain complex #define expressions
    // (e.g., referencing enum constants) that the CParserFacade cannot evaluate.
    auto enums_result = driver_->parse_enums();
    if (!enums_result.has_value()) {
        load_failed_ = true;
        return ChainableResult<void>{enums_result};
    }
    for (const auto &enum_decl : enums_result.value()) {
        for (const auto &member : enum_decl.members()) {
            auto insert_result = try_add_encounter_entry(member);
            if (!insert_result.has_value()) {
                return insert_result;
            }
        }
    }

    if (name_to_value_.empty()) {
        load_failed_ = true;
        return FormattableError{
            "{}: no encounter type definitions exist in file.", FormatParam{header_path_.string(), Style::bold}};
    }

    return {};
}

} // namespace porytiles2
