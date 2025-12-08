#include "porytiles2/domain/services/palette_validator.hpp"

#include <set>
#include <string>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace {

using namespace porytiles2;

constexpr auto pal_porytiles_transparency = "pal-porytiles-transparency";
constexpr auto pal_porytiles_slot0 = "pal-porytiles-slot0";
constexpr auto pal_porymap_transparency = "pal-porymap-transparency";
constexpr auto pal_porymap_slot0 = "pal-porymap-slot0";
constexpr auto pal_hint_duplicate_color = "pal-hint-duplicate-color";
constexpr auto pal_hint_transparency = "pal-hint-transparency";

std::string pal_filename(std::size_t pal_index)
{
    return pad_two_digits(pal_index) + ".pal";
}

ChainableResult<void> validate_single_override(
    const Palette<Rgba32, pal::max_size> &pal,
    std::size_t pal_index,
    const Rgba32 &extrinsic_transparency,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const PalettePrinter *pal_printer)
{
    bool hit_error = false;
    const std::string filename = pal_filename(pal_index);

    // Check 1: Slot 0 should match extrinsic transparency (warning only)
    if (!pal.is_wildcard(0)) {
        const Rgba32 slot0_color = pal.slot_zero_color();
        if (!slot0_color.is_extrinsically_transparent(extrinsic_transparency)) {
            std::vector<std::string> warning_lines;
            warning_lines.emplace_back(format->format(
                "Porytiles palette '{}' slot 0 color '{}' does not match extrinsic transparency '{}'",
                FormatParam{filename, Style::bold},
                FormatParam{slot0_color.to_jasc_str(), Style::bold},
                FormatParam{extrinsic_transparency.to_jasc_str(), Style::bold}));
            warning_lines.emplace_back("Slot 0 is typically reserved for the transparency color.");
            warning_lines.emplace_back("If you are using slot 0 for a .pla blend color, you can ignore this warning.");
            diag->warn(pal_porytiles_slot0, warning_lines);
        }
    }

    // Check 2: Non-slot-0 positions cannot contain extrinsic transparency
    for (std::size_t slot = 1; slot < pal.size(); ++slot) {
        if (pal.is_wildcard(slot)) {
            continue;
        }
        const Rgba32 color = pal.at(slot);
        if (color.is_extrinsically_transparent(extrinsic_transparency)) {
            hit_error = true;
            std::vector<std::string> error_lines;
            error_lines.emplace_back(format->format(
                "Porytiles palette '{}' slot '{}' contains extrinsic transparency color '{}'",
                FormatParam{filename, Style::bold},
                FormatParam{slot, Style::bold},
                FormatParam{color.to_jasc_str(), Style::bold}));
            error_lines.emplace_back("Extrinsic transparency is not allowed in non-slot-0 positions.");
            diag->err(pal_porytiles_transparency, error_lines);

            // Print the palette for context
            std::vector<std::string> note_lines;
            note_lines.emplace_back(format->format("palette '{}' contents:", FormatParam{filename, Style::bold}));
            std::ranges::copy(pal_printer->print_rgba_palette(pal), std::back_inserter(note_lines));
            diag->note(pal_porytiles_transparency, note_lines);
        }
    }

    if (hit_error) {
        return FormattableError{
            "{}: validation failed for palette override '{}'",
            FormatParam{pal_porytiles_transparency, Style::bold},
            FormatParam{filename, Style::bold}};
    }

    return {};
}

ChainableResult<void> validate_single_porymap_palette(
    const Palette<Rgba32, pal::max_size> &pal,
    std::size_t pal_index,
    const Rgba32 &extrinsic_transparency,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const PalettePrinter *pal_printer)
{
    bool hit_error = false;
    const std::string filename = pal_filename(pal_index);

    // Check 1: Slot 0 should match extrinsic transparency (warning only)
    if (!pal.is_wildcard(0)) {
        const Rgba32 slot0_color = pal.slot_zero_color();
        if (!slot0_color.is_extrinsically_transparent(extrinsic_transparency)) {
            std::vector<std::string> warning_lines;
            warning_lines.emplace_back(format->format(
                "Porymap palette '{}' slot 0 color '{}' does not match extrinsic transparency '{}'",
                FormatParam{filename, Style::bold},
                FormatParam{slot0_color.to_jasc_str(), Style::bold},
                FormatParam{extrinsic_transparency.to_jasc_str(), Style::bold}));
            warning_lines.emplace_back("Slot 0 is typically reserved for the transparency color.");
            warning_lines.emplace_back("If you are using slot 0 for a .pla blend color, you can ignore this warning.");
            diag->warn(pal_porymap_slot0, warning_lines);
        }
    }

    // Check 2: Non-slot-0 positions cannot contain extrinsic transparency
    for (std::size_t slot = 1; slot < pal.size(); ++slot) {
        if (pal.is_wildcard(slot)) {
            continue;
        }
        const Rgba32 color = pal.at(slot);
        if (color.is_extrinsically_transparent(extrinsic_transparency)) {
            hit_error = true;
            std::vector<std::string> error_lines;
            error_lines.emplace_back(format->format(
                "Porymap palette '{}' slot '{}' contains extrinsic transparency color '{}'",
                FormatParam{filename, Style::bold},
                FormatParam{slot, Style::bold},
                FormatParam{color.to_jasc_str(), Style::bold}));
            error_lines.emplace_back("Extrinsic transparency is not allowed in non-slot-0 positions.");
            diag->err(pal_porymap_transparency, error_lines);

            // Print the palette for context
            std::vector<std::string> note_lines;
            note_lines.emplace_back(format->format("palette '{}' contents:", FormatParam{filename, Style::bold}));
            std::ranges::copy(pal_printer->print_rgba_palette(pal), std::back_inserter(note_lines));
            diag->note(pal_porymap_transparency, note_lines);
        }
    }

    if (hit_error) {
        return FormattableError{
            "{}: validation failed for Porymap palette '{}'",
            FormatParam{pal_porymap_transparency, Style::bold},
            FormatParam{filename, Style::bold}};
    }

    return {};
}

ChainableResult<void> validate_single_hint(
    const PaletteHint &hint,
    const Rgba32 &extrinsic_transparency,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const PalettePrinter *pal_printer)
{
    bool hit_error = false;
    const Palette<Rgba32> &pal = hint.pal();
    const std::string &hint_name = hint.name();

    std::set<Rgba32> seen_colors;
    std::vector<std::size_t> violating_slots;

    for (std::size_t slot = 0; slot < pal.size(); ++slot) {
        if (pal.is_wildcard(slot)) {
            continue;
        }
        const Rgba32 color = pal.at(slot);

        // Check 1: Extrinsic transparency not allowed in hints
        if (color.is_extrinsically_transparent(extrinsic_transparency)) {
            hit_error = true;
            violating_slots.push_back(slot);
            std::vector<std::string> error_lines;
            error_lines.emplace_back(format->format(
                "palette hint '{}' slot '{}' contains extrinsic transparency color '{}'",
                FormatParam{hint_name, Style::bold},
                FormatParam{slot, Style::bold},
                FormatParam{color.to_jasc_str(), Style::bold}));
            error_lines.emplace_back("Extrinsic transparency is not allowed in palette hints.");
            diag->err(pal_hint_transparency, error_lines);
        }

        // Check 2: No duplicate colors
        if (seen_colors.contains(color)) {
            hit_error = true;
            violating_slots.push_back(slot);
            std::vector<std::string> error_lines;
            error_lines.emplace_back(format->format(
                "palette hint '{}' contains duplicate color '{}' at slot '{}'",
                FormatParam{hint_name, Style::bold},
                FormatParam{color.to_jasc_str(), Style::bold},
                FormatParam{slot, Style::bold}));
            error_lines.emplace_back("Duplicate colors are not allowed in palette hints.");
            diag->err(pal_hint_duplicate_color, error_lines);
        }
        seen_colors.insert(color);
    }

    if (hit_error) {
        // Print the palette with violating slots highlighted
        std::vector<std::string> note_lines;
        note_lines.emplace_back(format->format("palette hint '{}' contents:", FormatParam{hint_name, Style::bold}));
        note_lines.emplace_back();
        std::ranges::copy(
            pal_printer->print_rgba_palette_with_highlights(pal, violating_slots), std::back_inserter(note_lines));
        diag->note(pal_hint_transparency, note_lines);

        return FormattableError{"validation failed for palette hint '{}'", FormatParam{hint_name, Style::bold}};
    }

    return {};
}

ChainableResult<void> validate_porymap_palettes(
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
    const Rgba32 &extrinsic_transparency,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const PalettePrinter *pal_printer)
{
    std::vector<std::string> error_messages;

    for (std::size_t pal_index = 0; pal_index < pals.size(); ++pal_index) {
        const auto result = validate_single_porymap_palette(
            pals.at(pal_index), pal_index, extrinsic_transparency, format, diag, pal_printer);
        if (!result.has_value()) {
            error_messages.push_back(result.error().join(*format));
        }
    }

    if (!error_messages.empty()) {
        std::vector<std::string> combined_message{};
        combined_message.reserve(error_messages.size());
        for (const auto &msg : error_messages) {
            combined_message.emplace_back(msg);
        }
        return FormattableError{combined_message};
    }

    return {};
}

ChainableResult<void> validate_overrides(
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &overrides,
    const Rgba32 &extrinsic_transparency,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const PalettePrinter *pal_printer)
{
    std::vector<std::string> error_messages;

    for (std::size_t pal_index = 0; pal_index < overrides.size(); ++pal_index) {
        const auto &maybe_pal = overrides.at(pal_index);
        if (!maybe_pal.has_value()) {
            continue;
        }
        const auto result =
            validate_single_override(maybe_pal.value(), pal_index, extrinsic_transparency, format, diag, pal_printer);
        if (!result.has_value()) {
            error_messages.push_back(result.error().join(*format));
        }
    }

    if (!error_messages.empty()) {
        std::vector<std::string> combined_message{};
        combined_message.reserve(error_messages.size());
        for (const auto &msg : error_messages) {
            combined_message.emplace_back(msg);
        }
        return FormattableError{combined_message};
    }

    return {};
}

ChainableResult<void> validate_hints(
    const std::vector<PaletteHint> &hints,
    const Rgba32 &extrinsic_transparency,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const PalettePrinter *pal_printer)
{
    std::vector<std::string> error_messages;

    for (const auto &hint : hints) {
        const auto result = validate_single_hint(hint, extrinsic_transparency, format, diag, pal_printer);
        if (!result.has_value()) {
            error_messages.push_back(result.error().join(*format));
        }
    }

    if (!error_messages.empty()) {
        std::vector<std::string> combined_message{};
        combined_message.reserve(error_messages.size());
        for (const auto &msg : error_messages) {
            combined_message.emplace_back(msg);
        }
        return FormattableError{combined_message};
    }

    return {};
}

} // namespace

namespace porytiles2 {

ChainableResult<void> PaletteValidator::validate_primary(
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &porymap_pals,
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &overrides,
    const std::vector<PaletteHint> &hints) const
{
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset_scope_, void);

    std::vector<std::string> error_messages;

    // Run Porymap palette validation
    const auto porymap_result =
        validate_porymap_palettes(porymap_pals, extrinsic_transparency, format_, diag_, pal_printer_);
    if (!porymap_result.has_value()) {
        error_messages.push_back(porymap_result.error().join(*format_));
    }

    // Run Porytiles override palette validation
    const auto overrides_result = validate_overrides(overrides, extrinsic_transparency, format_, diag_, pal_printer_);
    if (!overrides_result.has_value()) {
        error_messages.push_back(overrides_result.error().join(*format_));
    }

    // Run palette hint validation
    const auto hints_result = validate_hints(hints, extrinsic_transparency, format_, diag_, pal_printer_);
    if (!hints_result.has_value()) {
        error_messages.push_back(hints_result.error().join(*format_));
    }

    // If any validation failed, return all error messages
    if (!error_messages.empty()) {
        std::vector<std::string> combined_message{};
        combined_message.reserve(error_messages.size() + 1);
        combined_message.emplace_back("palette validation failed with the following error(s):");
        for (const auto &msg : error_messages) {
            combined_message.emplace_back("  - " + msg);
        }
        return FormattableError{combined_message};
    }

    return {};
}

} // namespace porytiles2
