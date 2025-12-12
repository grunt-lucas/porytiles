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

constexpr auto pal_porytiles_transparency = "porytiles-pal-transparency";
constexpr auto pal_porytiles_slot0 = "porytiles-pal-slot0";
constexpr auto pal_porymap_transparency = "porymap-pal-transparency";
constexpr auto pal_porymap_slot0 = "porymap-pal-slot0";
constexpr auto pal_hint_duplicate_color = "pal-hint-duplicate-color";
constexpr auto pal_hint_transparency = "pal-hint-transparency";

template <std::size_t N>
void print_palette_with_highlights_note(
    const std::string &note_tag,
    const Palette<Rgba32, N> &pal,
    const std::string &label,
    const std::vector<std::size_t> &violating_slots,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const PalettePrinter *pal_printer)
{
    std::vector<std::string> note_lines;
    note_lines.emplace_back(format->format("'{}' contents:", FormatParam{label, Style::bold}));
    note_lines.emplace_back();
    std::ranges::copy(
        pal_printer->print_rgba_palette_with_highlights(pal, violating_slots), std::back_inserter(note_lines));
    diag->note(note_tag, note_lines);
}

void print_extrinsic_transparency_note(
    const std::string &note_tag,
    const ConfigValue<Rgba32> &extrinsic_transparency,
    const TextFormatter *format,
    const UserDiagnostics *diag)
{
    std::vector<std::string> config_note_text;
    config_note_text.push_back(format->format(
        "extrinsic transparency is '{}' due to configuration", FormatParam{extrinsic_transparency, Style::bold}));
    config_note_text.emplace_back("");
    std::ranges::copy(extrinsic_transparency.prettify(*format), std::back_inserter(config_note_text));
    diag->note(note_tag, config_note_text);
}

ChainableResult<void> validate_single_porytiles_pal(
    const Palette<Rgba32, pal::max_size> &pal,
    std::size_t pal_index,
    const DomainConfig *config,
    const std::string &tileset_scope,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const PalettePrinter *pal_printer)
{
    PT_UNWRAP_TILESET_CONFIG_PTR(config, extrinsic_transparency, tileset_scope, void);

    bool hit_error = false;
    const std::string filename = pal_filename(pal_index);
    std::vector<std::size_t> violating_slots;

    // Check 1: Slot 0 should match extrinsic transparency (warning only)
    if (!pal.is_wildcard(0)) {
        const Rgba32 slot0_color = pal.slot_zero_color();
        if (!slot0_color.is_extrinsically_transparent(extrinsic_transparency)) {
            // Build the warning text
            std::vector<std::string> warning_text;
            warning_text.emplace_back(format->format(
                "Porytiles palette '{}' slot 0 color '{}' does not match extrinsic transparency '{}'",
                FormatParam{filename, Style::bold},
                FormatParam{slot0_color.to_jasc_str(), Style::bold},
                FormatParam{extrinsic_transparency.value().to_jasc_str(), Style::bold}));
            warning_text.emplace_back("Slot 0 is typically reserved for the transparency color.");
            warning_text.emplace_back("If you are using slot 0 for a .pla blend color, you can ignore this warning.");
            diag->warn(pal_porytiles_slot0, warning_text);

            // Print the palette and config notes.
            print_palette_with_highlights_note(
                pal_porytiles_slot0, pal, filename, std::vector<std::size_t>{0}, format, diag, pal_printer);
            print_extrinsic_transparency_note(pal_porytiles_slot0, extrinsic_transparency, format, diag);
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
            violating_slots.push_back(slot);
            std::vector<std::string> error_lines;
            error_lines.emplace_back(format->format(
                "Porytiles palette '{}' slot '{}' contains extrinsic transparency color '{}'",
                FormatParam{filename, Style::bold},
                FormatParam{slot, Style::bold},
                FormatParam{color.to_jasc_str(), Style::bold}));
            error_lines.emplace_back("Extrinsic transparency is not allowed in non-slot-0 positions.");
            diag->err(pal_porytiles_transparency, error_lines);
        }
    }

    if (hit_error) {
        // Print the palette and config notes
        print_palette_with_highlights_note(
            pal_porytiles_transparency, pal, filename, violating_slots, format, diag, pal_printer);
        print_extrinsic_transparency_note(pal_porytiles_transparency, extrinsic_transparency, format, diag);

        return FormattableError{"validation failed for Porytiles palette '{}'", FormatParam{filename, Style::bold}};
    }

    return {};
}

ChainableResult<void> validate_single_porymap_pal(
    const Palette<Rgba32, pal::max_size> &pal,
    std::size_t pal_index,
    const DomainConfig *config,
    const std::string &tileset_scope,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const PalettePrinter *pal_printer)
{
    PT_UNWRAP_TILESET_CONFIG_PTR(config, extrinsic_transparency, tileset_scope, void);

    bool hit_error = false;
    const std::string filename = pal_filename(pal_index);
    std::vector<std::size_t> violating_slots;

    if (pal.is_wildcard(0)) {
        panic("unexpected wildcard in Porymap palette slot 0");
    }

    // Check 1: Slot 0 should match extrinsic transparency (warning only)
    const Rgba32 slot0_color = pal.slot_zero_color();
    if (!slot0_color.is_extrinsically_transparent(extrinsic_transparency)) {
        std::vector<std::string> warning_lines;
        warning_lines.emplace_back(format->format(
            "Porymap palette '{}' slot 0 color '{}' does not match extrinsic transparency '{}'",
            FormatParam{filename, Style::bold},
            FormatParam{slot0_color.to_jasc_str(), Style::bold},
            FormatParam{extrinsic_transparency.value().to_jasc_str(), Style::bold}));
        warning_lines.emplace_back("Slot 0 is typically reserved for the transparency color.");
        warning_lines.emplace_back("If you are using slot 0 for a .pla blend color, you can ignore this warning.");
        diag->warn(pal_porymap_slot0, warning_lines);

        // Print the palette and config notes.
        print_palette_with_highlights_note(
            pal_porymap_slot0, pal, filename, std::vector<std::size_t>{0}, format, diag, pal_printer);
        print_extrinsic_transparency_note(pal_porymap_slot0, extrinsic_transparency, format, diag);
    }

    // Check 2: Non-slot-0 positions cannot contain extrinsic transparency
    for (std::size_t slot = 1; slot < pal.size(); ++slot) {
        if (pal.is_wildcard(slot)) {
            panic("unexpected wildcard in Porymap palette");
        }
        const Rgba32 color = pal.at(slot);
        if (color.is_extrinsically_transparent(extrinsic_transparency)) {
            hit_error = true;
            violating_slots.push_back(slot);
            std::vector<std::string> error_lines;
            error_lines.emplace_back(format->format(
                "Porymap palette '{}' slot '{}' contains extrinsic transparency color '{}'",
                FormatParam{filename, Style::bold},
                FormatParam{slot, Style::bold},
                FormatParam{color.to_jasc_str(), Style::bold}));
            error_lines.emplace_back("Extrinsic transparency is not allowed in non-slot-0 positions.");
            diag->err(pal_porymap_transparency, error_lines);
        }
    }

    if (hit_error) {
        // Print the palette with violating slots highlighted
        print_palette_with_highlights_note(
            pal_porymap_transparency, pal, filename, violating_slots, format, diag, pal_printer);
        print_extrinsic_transparency_note(pal_porymap_transparency, extrinsic_transparency, format, diag);

        return FormattableError{"validation failed for Porymap palette '{}'", FormatParam{filename, Style::bold}};
    }

    return {};
}

ChainableResult<void> validate_single_hint(
    const PaletteHint &hint,
    const DomainConfig *config,
    const std::string &tileset_scope,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const PalettePrinter *pal_printer)
{
    PT_UNWRAP_TILESET_CONFIG_PTR(config, extrinsic_transparency, tileset_scope, void);

    /*
     * TODO: we need to validate that hints are size 15 or less, should we do this here or at the config loader level?
     */

    bool hit_any_error = false;
    const Palette<Rgba32> &pal = hint.pal();
    const std::string &hint_name = hint.name();

    std::set<Rgba32> seen_colors;
    std::vector<std::size_t> violating_slots;

    // Check 1: Extrinsic transparency not allowed in hints
    for (std::size_t slot = 0; slot < pal.size(); ++slot) {
        if (pal.is_wildcard(slot)) {
            continue;
        }
        const Rgba32 color = pal.at(slot);

        if (color.is_extrinsically_transparent(extrinsic_transparency)) {
            hit_any_error = true;
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
    }
    if (!violating_slots.empty()) {
        print_palette_with_highlights_note(
            pal_hint_transparency, pal, hint_name, violating_slots, format, diag, pal_printer);
        print_extrinsic_transparency_note(pal_hint_transparency, extrinsic_transparency, format, diag);
    }
    violating_slots.clear();

    // Check 2: No duplicate colors
    for (std::size_t slot = 0; slot < pal.size(); ++slot) {
        if (pal.is_wildcard(slot)) {
            continue;
        }
        const Rgba32 color = pal.at(slot);

        if (seen_colors.contains(color)) {
            hit_any_error = true;
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
    if (!violating_slots.empty()) {
        print_palette_with_highlights_note(
            pal_hint_duplicate_color, pal, hint_name, violating_slots, format, diag, pal_printer);
    }

    if (hit_any_error) {
        return FormattableError{"validation failed for palette hint '{}'", FormatParam{hint_name, Style::bold}};
    }
    return {};
}

} // namespace

namespace porytiles2 {

ChainableResult<void> PaletteValidator::validate_primary(
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &porymap_pals,
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &porytiles_pals,
    const std::vector<PaletteHint> &hints) const
{
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset_scope_, void);

    std::vector<std::string> error_messages;

    if (run_porymap_validations_) {
        // Validate Porymap palettes
        for (std::size_t pal_index = 0; pal_index < porymap_pals.size(); ++pal_index) {
            const auto result = validate_single_porymap_pal(
                porymap_pals.at(pal_index), pal_index, config_, tileset_scope_, format_, diag_, pal_printer_);
            if (!result.has_value()) {
                error_messages.push_back(result.error().join(*format_));
            }
        }
    }

    // Validate Porytiles palettes
    for (std::size_t pal_index = 0; pal_index < porytiles_pals.size(); ++pal_index) {
        const auto &maybe_pal = porytiles_pals.at(pal_index);
        if (!maybe_pal.has_value()) {
            continue;
        }
        const auto result = validate_single_porytiles_pal(
            maybe_pal.value(), pal_index, config_, tileset_scope_, format_, diag_, pal_printer_);
        if (!result.has_value()) {
            error_messages.push_back(result.error().join(*format_));
        }
    }

    // Validate palette hints
    for (const auto &hint : hints) {
        const auto result = validate_single_hint(hint, config_, tileset_scope_, format_, diag_, pal_printer_);
        if (!result.has_value()) {
            error_messages.push_back(result.error().join(*format_));
        }
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
