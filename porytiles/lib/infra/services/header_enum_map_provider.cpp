#include "porytiles/infra/services/header_enum_map_provider.hpp"

#include <bit>
#include <utility>

#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/text/file_highlight_printer.hpp"

namespace porytiles {

namespace {

/// @brief Builds a multi-line FormattableError showing both duplicate locations with source context.
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
ChainableResult<void> HeaderEnumMapProvider::try_add_entry(const Entry &entry) const
{
    const auto &name = entry.name();

    // Filter: must start with the configured prefix
    if (!name.starts_with(definition_.prefix)) {
        return {};
    }

    // Filter: skip names the definition excludes
    if (definition_.skipped.contains(name)) {
        return {};
    }

    auto raw_value = entry.int_value();
    const auto &new_pos = entry.position();

    // A parsed value outside the field's range is a hard error, not a name to drop quietly. Dropping it would resurface
    // later as a baffling "no such name" lookup failure; the real problem is that the field's mask is too narrow for
    // the header's constants (or the name is a sentinel that belongs in the skipped set).
    if (raw_value < 0 || raw_value > definition_.max_value) {
        load_failed_ = true;
        std::vector<std::string> lines;
        FileHighlightPrinter printer{format_};

        lines.push_back(format_->format(
            "{}:{}:{}: '{}' has value '{}', which does not fit in the {}-bit field '{}'.",
            FormatParam{header_path_, Style::bold},
            new_pos.line,
            new_pos.column,
            FormatParam{name, Style::bold},
            FormatParam{raw_value, Style::bold},
            FormatParam{std::bit_width(definition_.max_value)},
            FormatParam{definition_.field_display_name, Style::bold}));

        assert_or_panic(new_pos.line > 0, "new_pos.line must be positive (1-based)");
        assert_or_panic(new_pos.line <= driver_->file_lines().size(), "new_pos.line exceeds file bounds");
        assert_or_panic(new_pos.column > 0, "new_pos.column must be positive (1-based)");
        auto context = printer.print(driver_->file_lines(), new_pos.line - 1, new_pos.column - 1);
        for (auto &line : context) {
            lines.push_back(std::move(line));
        }

        lines.emplace_back("");
        lines.push_back(format_->format(
            "{} widen the field's mask to cover this value, or add '{}' to the provider's skipped names to ignore it.",
            FormatParam{"note:", Style::cyan | Style::bold},
            FormatParam{name, Style::bold}));

        return FormattableError{std::move(lines)};
    }

    auto value = static_cast<std::uint32_t>(raw_value);

    // Check for duplicate name
    if (name_to_value_.contains(name)) {
        load_failed_ = true;
        const auto &orig_pos = name_to_position_.at(name);
        return make_duplicate_error(
            format_->format(
                "{}:{}:{}: duplicate {} name '{}'.",
                FormatParam{header_path_, Style::bold},
                new_pos.line,
                new_pos.column,
                FormatParam{definition_.field_display_name},
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
                "{}:{}:{}: duplicate {} value '{}': both '{}' and '{}' have this value.",
                FormatParam{header_path_, Style::bold},
                new_pos.line,
                new_pos.column,
                FormatParam{definition_.field_display_name},
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

ChainableResult<std::uint32_t> HeaderEnumMapProvider::lookup(const std::string &name) const
{
    if (!name.starts_with(definition_.prefix)) {
        return FormattableError{
            "Invalid {} name '{}': expected prefix '{}'.",
            FormatParam{definition_.field_display_name},
            FormatParam{name, Style::bold},
            FormatParam{definition_.prefix, Style::bold}};
    }

    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::uint32_t>{
            FormattableError{
                "Provider lookup for field '{}' failed.", FormatParam{definition_.field_display_name, Style::bold}},
            load_result};
    }

    const auto it = name_to_value_.find(name);
    if (it == name_to_value_.end()) {
        return FormattableError{
            "No {} named '{}' exists in '{}'.",
            FormatParam{definition_.field_display_name},
            FormatParam{name, Style::bold},
            FormatParam{header_path_.string(), Style::bold}};
    }
    return it->second;
}

ChainableResult<std::string> HeaderEnumMapProvider::lookup(std::uint32_t value) const
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::string>{
            FormattableError{
                "Provider lookup for field '{}' failed.", FormatParam{definition_.field_display_name, Style::bold}},
            load_result};
    }

    const auto it = value_to_name_.find(value);
    if (it == value_to_name_.end()) {
        return FormattableError{
            "No {} with value '{}' exists in '{}'.",
            FormatParam{definition_.field_display_name},
            FormatParam{value, Style::bold},
            FormatParam{header_path_.string(), Style::bold}};
    }
    return it->second;
}

ChainableResult<void> HeaderEnumMapProvider::ensure_loaded() const
{
    if (loaded_) {
        if (load_failed_) {
            return FormattableError{
                "Header file for field '{}' previously failed to load.",
                FormatParam{definition_.field_display_name, Style::bold}};
        }
        return {};
    }

    loaded_ = true;

    // Create and store CParserFacade for rich error formatting with source context
    driver_ = std::make_unique<CParserFacade>(header_path_, format_);

    // Parse #define statements when the format admits them
    if (definition_.format == HeaderFormat::defines_only || definition_.format == HeaderFormat::either) {
        auto defines_result = driver_->parse_defines();
        if (!defines_result.has_value()) {
            load_failed_ = true;
            return ChainableResult<void>{defines_result};
        }
        for (const auto &def : defines_result.value()) {
            if (!def.has_int_value()) {
                continue;
            }
            auto insert_result = try_add_entry(def);
            if (!insert_result.has_value()) {
                return insert_result;
            }
        }
    }

    // Parse enum declarations when the format admits them
    if (definition_.format == HeaderFormat::enums_only || definition_.format == HeaderFormat::either) {
        auto enums_result = driver_->parse_enums();
        if (!enums_result.has_value()) {
            load_failed_ = true;
            return ChainableResult<void>{enums_result};
        }
        for (const auto &enum_decl : enums_result.value()) {
            for (const auto &member : enum_decl.members()) {
                auto insert_result = try_add_entry(member);
                if (!insert_result.has_value()) {
                    return insert_result;
                }
            }
        }
    }

    if (name_to_value_.empty()) {
        load_failed_ = true;
        return FormattableError{
            "Field '{}' declared provider prefix '{}' in '{}' but no matching names were found.",
            FormatParam{definition_.field_display_name, Style::bold},
            FormatParam{definition_.prefix, Style::bold},
            FormatParam{header_path_.string(), Style::bold}};
    }

    return {};
}

ProviderMap build_provider_map(
    const std::filesystem::path &project_root,
    const Schema &schema,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag)
{
    // Membership contract: the map contains exactly the schema's has_provider() fields, so downstream code
    // can key raw-vs-provider handling off has_provider() and treat a missing map entry as an internal bug.
    ProviderMap providers{};
    for (const Field &field : schema.fields()) {
        if (!field.has_provider()) {
            continue;
        }
        const ProviderDefinition &definition = field.provider_definition();
        providers.emplace(
            field.name(),
            std::make_unique<HeaderEnumMapProvider>(
                project_root / definition.header,
                definition.to_enum_definition(field.name(), field.max_value()),
                format,
                diag));
    }
    return providers;
}

} // namespace porytiles
