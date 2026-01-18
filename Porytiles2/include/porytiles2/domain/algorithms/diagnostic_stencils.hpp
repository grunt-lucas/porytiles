#pragma once

#include <ranges>
#include <string>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Prints a note explaining the global color count limit for primary tileset compilation.
 *
 * @details
 * This function creates and prints a standardized note explaining how the global color count limit is calculated. It is
 * used in error messages when the user exceeds the maximum number of unique colors allowed in a tileset.
 *
 * The generated note includes:
 * - A statement of the limit value
 * - The formula for calculating the limit (num_pals_in_primary * nontransparent_colors_per_pal)
 * - The prettified configuration value showing the source of the num_pals setting
 *
 * @param format The text formatter for styling output
 * @param diag The user diagnostics interface for printing notes
 * @param note_tag The tag to associate with the note for diagnostic filtering
 * @param color_count_limit The calculated color count limit value
 * @param num_pals The configuration value for the number of palettes in primary
 */
inline void print_global_color_limit_definition(
    const TextFormatter &format,
    const UserDiagnostics &diag,
    const std::string &note_tag,
    std::size_t color_count_limit,
    const ConfigValue<std::size_t> &num_pals)
{
    std::vector<std::string> note_text;
    note_text.push_back(format.format(
        "unique color count limit is '{}' due to configuration", FormatParam{color_count_limit, Style::bold}));
    note_text.emplace_back("");
    note_text.emplace_back("Color limit definition:");
    note_text.push_back(format.format(
        "{} * {}:",
        FormatParam{num_pals.name(), Style::bold | Style::yellow},
        FormatParam{"nontransparent_colors_per_pal", Style::bold}));
    note_text.push_back(format.format(
        "{} * {} = {}",
        FormatParam{num_pals.value(), Style::bold | Style::yellow},
        FormatParam{(pal::max_size - 1), Style::bold},
        FormatParam{color_count_limit, Style::bold}));
    note_text.emplace_back("");
    std::ranges::copy(num_pals.prettify(format), std::back_inserter(note_text));
    diag.note(note_tag, note_text);
}

/**
 * @brief Prints a diagnostic note displaying a Porymap palette with highlighted violating slots.
 *
 * @details
 * This function generates and prints a formatted diagnostic note showing a Porymap palette with specific color slots
 * highlighted to indicate violations. It is used to provide visual feedback when palette-related errors occur during
 * tileset compilation, specifically for palettes originating from Porymap assets.
 *
 * The generated note includes:
 * - A header identifying the Porymap palette by label and custom message
 * - The palette rendered with violating slots visually highlighted
 *
 * @tparam N The size of the palette (number of color slots)
 * @param format The text formatter for styling output
 * @param diag The user diagnostics interface for printing notes
 * @param pal_printer The palette printer for rendering palette visualizations
 * @param note_tag The tag to associate with the note for diagnostic filtering
 * @param message A custom message describing the issue with the palette
 * @param pal The palette to display
 * @param pal_label The human-readable label identifying this palette
 * @param violating_slots The indices of slots that should be highlighted as violations
 */
template <std::size_t N>
void print_porymap_pal_with_highlights_note(
    const TextFormatter &format,
    const UserDiagnostics &diag,
    const PalettePrinter &pal_printer,
    const std::string &note_tag,
    const std::string &message,
    const Palette<Rgba32, N> &pal,
    const std::string &pal_label,
    const std::vector<std::size_t> &violating_slots)
{
    std::vector<std::string> note_lines;
    note_lines.emplace_back(
        format.format("Porymap palette '{}': {}:", FormatParam{pal_label, Style::bold}, FormatParam{message}));
    note_lines.emplace_back();
    std::ranges::copy(pal_printer.print_rgba_pal_with_highlights(pal, violating_slots), std::back_inserter(note_lines));
    diag.note(note_tag, note_lines);
}

/**
 * @brief Prints a diagnostic note displaying a Porytiles palette with highlighted violating slots.
 *
 * @details
 * This function generates and prints a formatted diagnostic note showing a Porytiles palette with specific color slots
 * highlighted to indicate violations. It is used to provide visual feedback when palette-related errors occur during
 * tileset compilation.
 *
 * The generated note includes:
 * - A header identifying the Porytiles palette by label and custom message
 * - The palette rendered with violating slots visually highlighted
 *
 * @tparam N The size of the palette (number of color slots)
 * @param format The text formatter for styling output
 * @param diag The user diagnostics interface for printing notes
 * @param pal_printer The palette printer for rendering palette visualizations
 * @param note_tag The tag to associate with the note for diagnostic filtering
 * @param message A custom message describing the issue with the palette
 * @param pal The palette to display
 * @param pal_label The human-readable label identifying this palette
 * @param violating_slots The indices of slots that should be highlighted as violations
 */
template <std::size_t N>
void print_porytiles_pal_with_highlights_note(
    const TextFormatter &format,
    const UserDiagnostics &diag,
    const PalettePrinter &pal_printer,
    const std::string &note_tag,
    const std::string &message,
    const Palette<Rgba32, N> &pal,
    const std::string &pal_label,
    const std::vector<std::size_t> &violating_slots)
{
    std::vector<std::string> note_lines;
    note_lines.emplace_back(
        format.format("Porytiles palette '{}': {}:", FormatParam{pal_label, Style::bold}, FormatParam{message}));
    note_lines.emplace_back();
    std::ranges::copy(pal_printer.print_rgba_pal_with_highlights(pal, violating_slots), std::back_inserter(note_lines));
    diag.note(note_tag, note_lines);
}

/**
 * @brief Prints a diagnostic note displaying a palette hint with highlighted violating slots.
 *
 * @details
 * This function generates and prints a formatted diagnostic note showing a palette hint with specific color slots
 * highlighted to indicate violations. Palette hints are user-provided color specifications that guide the palette
 * assignment algorithm, and this function helps diagnose issues when hints contain invalid or conflicting colors.
 *
 * The generated note includes:
 * - A header identifying the palette hint by label and custom message
 * - The palette hint rendered with violating slots visually highlighted
 *
 * @param format The text formatter for styling output
 * @param diag The user diagnostics interface for printing notes
 * @param pal_printer The palette printer for rendering palette visualizations
 * @param note_tag The tag to associate with the note for diagnostic filtering
 * @param message A custom message describing the issue with the palette hint
 * @param hint The palette hint to display
 * @param pal_label The human-readable label identifying this palette hint
 * @param violating_slots The indices of slots that should be highlighted as violations
 */
inline void print_pal_hint_with_highlights_note(
    const TextFormatter &format,
    const UserDiagnostics &diag,
    const PalettePrinter &pal_printer,
    const std::string &note_tag,
    const std::string &message,
    const PaletteHint &hint,
    const std::string &pal_label,
    const std::vector<std::size_t> &violating_slots)
{
    std::vector<std::string> note_lines;
    note_lines.emplace_back(
        format.format("palette hint '{}': {}:", FormatParam{pal_label, Style::bold}, FormatParam{message}));
    note_lines.emplace_back();
    std::ranges::copy(
        pal_printer.print_pal_hint_with_highlights(hint, violating_slots), std::back_inserter(note_lines));
    diag.note(note_tag, note_lines);
}

/**
 * @brief Prints a diagnostic note explaining the configured extrinsic transparency color.
 *
 * @details
 * This function generates and prints a formatted diagnostic note explaining how the extrinsic transparency color is
 * configured. The extrinsic transparency is the color in input images that represents transparent pixels when the
 * alpha channel is not being used.
 *
 * The generated note includes:
 * - A statement of the configured extrinsic transparency color value
 * - The prettified configuration value showing the source of the setting
 *
 * @param format The text formatter for styling output
 * @param diag The user diagnostics interface for printing notes
 * @param note_tag The tag to associate with the note for diagnostic filtering
 * @param extrinsic_transparency The configuration value for the extrinsic transparency color
 */
inline void print_extrinsic_transparency_note(
    const TextFormatter &format,
    const UserDiagnostics &diag,
    const std::string &note_tag,
    const ConfigValue<Rgba32> &extrinsic_transparency)
{
    std::vector<std::string> config_note_text;
    config_note_text.push_back(format.format(
        "extrinsic transparency is '{}' due to configuration", FormatParam{extrinsic_transparency, Style::bold}));
    config_note_text.emplace_back("");
    std::ranges::copy(extrinsic_transparency.prettify(format), std::back_inserter(config_note_text));
    diag.note(note_tag, config_note_text);
}

} // namespace porytiles2
