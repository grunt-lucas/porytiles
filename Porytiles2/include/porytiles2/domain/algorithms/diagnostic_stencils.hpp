#pragma once

#include <ranges>
#include <string>
#include <vector>

#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Builds note lines explaining the global color count limit for primary tileset compilation.
 *
 * @details
 * This function creates and returns standardized note lines explaining how the global color count limit is calculated.
 * It is used in error messages when the user exceeds the maximum number of unique colors allowed in a tileset.
 *
 * The generated note includes:
 * - A statement of the limit value
 * - The formula for calculating the limit (num_pals_in_primary * nontransparent_colors_per_pal)
 * - The prettified configuration value showing the source of the num_pals setting
 *
 * @param format The TextFormatter for styling
 * @param color_count_limit The calculated color count limit value
 * @param num_pals The configuration value for the number of palettes in primary
 * @return Vector of formatted lines describing the color count limit configuration
 */
[[nodiscard]] inline std::vector<std::string> build_global_color_limit_lines(
    const TextFormatter &format, std::size_t color_count_limit, const ConfigValue<std::size_t> &num_pals)
{
    std::vector<std::string> lines;
    lines.push_back(format.format(
        "unique color count limit is '{}' due to configuration", FormatParam{color_count_limit, Style::bold}));
    lines.emplace_back("");
    lines.emplace_back("Color limit definition:");
    lines.push_back(format.format(
        "{} * {}:",
        FormatParam{num_pals.name(), Style::bold | Style::yellow},
        FormatParam{"nontransparent_colors_per_pal", Style::bold}));
    lines.push_back(format.format(
        "{} * {} = {}",
        FormatParam{num_pals.value(), Style::bold | Style::yellow},
        FormatParam{(pal::max_size - 1), Style::bold},
        FormatParam{color_count_limit, Style::bold}));
    lines.emplace_back("");
    std::ranges::copy(num_pals.prettify(format), std::back_inserter(lines));
    return lines;
}

/**
 * @brief Builds note lines displaying a Porymap palette with highlighted violating slots.
 *
 * @details
 * This function generates and returns formatted note lines showing a Porymap palette with specific color slots
 * highlighted to indicate violations. It is used to provide visual feedback when palette-related errors occur during
 * tileset compilation, specifically for palettes originating from Porymap assets.
 *
 * The generated note includes:
 * - A header identifying the Porymap palette by label and custom message
 * - The palette rendered with violating slots visually highlighted
 *
 * @tparam N The size of the palette (number of color slots)
 * @param format The TextFormatter for styling
 * @param pal_printer The palette printer for rendering palette visualizations
 * @param message A custom message describing the issue with the palette
 * @param pal The palette to display
 * @param pal_label The human-readable label identifying this palette
 * @param violating_slots The indices of slots that should be highlighted as violations
 * @return Vector of formatted lines describing the palette with highlights
 */
template <std::size_t N>
[[nodiscard]] std::vector<std::string> build_porymap_pal_highlight_lines(
    const TextFormatter &format,
    const PalettePrinter &pal_printer,
    const std::string &message,
    const Palette<Rgba32, N> &pal,
    const std::string &pal_label,
    const std::vector<std::size_t> &violating_slots)
{
    std::vector<std::string> lines;
    lines.emplace_back(
        format.format("Porymap palette '{}': {}:", FormatParam{pal_label, Style::bold}, FormatParam{message}));
    lines.emplace_back();
    std::ranges::copy(pal_printer.print_rgba_pal_with_highlights(pal, violating_slots), std::back_inserter(lines));
    return lines;
}

/**
 * @brief Builds note lines displaying a Porytiles palette with highlighted violating slots.
 *
 * @details
 * This function generates and returns formatted note lines showing a Porytiles palette with specific color slots
 * highlighted to indicate violations. It is used to provide visual feedback when palette-related errors occur during
 * tileset compilation.
 *
 * The generated note includes:
 * - A header identifying the Porytiles palette by label and custom message
 * - The palette rendered with violating slots visually highlighted
 *
 * @tparam N The size of the palette (number of color slots)
 * @param format The TextFormatter for styling
 * @param pal_printer The palette printer for rendering palette visualizations
 * @param message A custom message describing the issue with the palette
 * @param pal The palette to display
 * @param pal_label The human-readable label identifying this palette
 * @param violating_slots The indices of slots that should be highlighted as violations
 * @return Vector of formatted lines describing the palette with highlights
 */
template <std::size_t N>
[[nodiscard]] std::vector<std::string> build_porytiles_pal_highlight_lines(
    const TextFormatter &format,
    const PalettePrinter &pal_printer,
    const std::string &message,
    const Palette<Rgba32, N> &pal,
    const std::string &pal_label,
    const std::vector<std::size_t> &violating_slots)
{
    std::vector<std::string> lines;
    lines.emplace_back(
        format.format("Porytiles palette '{}': {}:", FormatParam{pal_label, Style::bold}, FormatParam{message}));
    lines.emplace_back();
    std::ranges::copy(pal_printer.print_rgba_pal_with_highlights(pal, violating_slots), std::back_inserter(lines));
    return lines;
}

/**
 * @brief Builds note lines displaying a palette hint with highlighted violating slots.
 *
 * @details
 * This function generates and returns formatted note lines showing a palette hint with specific color slots
 * highlighted to indicate violations. Palette hints are user-provided color specifications that guide the palette
 * assignment algorithm, and this function helps diagnose issues when hints contain invalid or conflicting colors.
 *
 * The generated note includes:
 * - A header identifying the palette hint by label and custom message
 * - The palette hint rendered with violating slots visually highlighted
 *
 * @param format The TextFormatter for styling
 * @param pal_printer The palette printer for rendering palette visualizations
 * @param message A custom message describing the issue with the palette hint
 * @param hint The palette hint to display
 * @param pal_label The human-readable label identifying this palette hint
 * @param violating_slots The indices of slots that should be highlighted as violations
 * @return Vector of formatted lines describing the palette hint with highlights
 */
[[nodiscard]] inline std::vector<std::string> build_pal_hint_highlight_lines(
    const TextFormatter &format,
    const PalettePrinter &pal_printer,
    const std::string &message,
    const PaletteHint &hint,
    const std::string &pal_label,
    const std::vector<std::size_t> &violating_slots)
{
    std::vector<std::string> lines;
    lines.emplace_back(
        format.format("palette hint '{}': {}:", FormatParam{pal_label, Style::bold}, FormatParam{message}));
    lines.emplace_back();
    std::ranges::copy(pal_printer.print_pal_hint_with_highlights(hint, violating_slots), std::back_inserter(lines));
    return lines;
}

/**
 * @brief Format a ConfigValue into diagnostic note lines.
 *
 * @details
 * Shared helper that constructs the standard format for displaying configuration values in note diagnostics. The
 * output includes a header line showing the config name and value, followed by a blank line, then the full
 * prettified configuration context.
 *
 * @tparam T The underlying type of the ConfigValue
 * @param format The TextFormatter for styling
 * @param config The ConfigValue to format
 * @return Vector of formatted lines ready for note output
 */
template <typename T>
[[nodiscard]] std::vector<std::string> format_config_note(const TextFormatter &format, const ConfigValue<T> &config)
{
    std::vector<std::string> lines{};
    lines.push_back(format.format(
        "'{}' is '{}' due to configuration:",
        FormatParam{config.name(), Style::bold},
        FormatParam{config.value(), Style::bold}));
    lines.emplace_back("");
    std::ranges::copy(config.prettify(format), std::back_inserter(lines));
    return lines;
}

/**
 * @brief Format a ConfigValue into diagnostic note lines with a separator.
 *
 * @details
 * This method is the same as format_config_note, but it includes a separator section above the config note. This is
 * useful for cases where the caller is already printing some other information and wants some visual separation between
 * that info and the config printout.
 *
 * @tparam T The underlying type of the ConfigValue
 * @param format The TextFormatter for styling
 * @param config The ConfigValue to format
 * @return Vector of formatted lines ready for note output
 */
template <typename T>
[[nodiscard]] std::vector<std::string>
format_config_note_with_separator(const TextFormatter &format, const ConfigValue<T> &config)
{
    std::vector<std::string> lines{};
    lines.emplace_back("");
    lines.emplace_back("--------");
    lines.emplace_back("");
    std::ranges::copy(format_config_note(format, config), std::back_inserter(lines));
    return lines;
}

} // namespace porytiles2
