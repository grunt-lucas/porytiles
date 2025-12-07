#pragma once

#include <ranges>
#include <string>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/config/config_value.hpp"

namespace porytiles2 {

/**
 * @brief Generates a note text explaining the global color count limit for primary tileset compilation.
 *
 * @details
 * This function creates a standardized note explaining how the global color count limit is calculated. It is used
 * in error messages when the user exceeds the maximum number of unique colors allowed in a tileset.
 *
 * The generated note includes:
 * - A statement of the limit value
 * - The formula for calculating the limit (num_pals_in_primary * nontransparent_colors_per_pal)
 * - The prettified configuration value showing the source of num_pals_in_primary
 *
 * @param format The text formatter for styling output
 * @param color_count_limit The calculated color count limit value
 * @param num_pals_in_primary The configuration value for the number of palettes in primary
 * @return A vector of strings representing the note text lines
 */
[[nodiscard]] inline std::vector<std::string> global_color_limit_definition(
    const TextFormatter &format, std::size_t color_count_limit, const ConfigValue<std::size_t> &num_pals_in_primary)
{
    std::vector<std::string> note_text;
    note_text.push_back(format.format(
        "unique color count limit is '{}' due to configuration", FormatParam{color_count_limit, Style::bold}));
    note_text.emplace_back("");
    note_text.emplace_back("Color limit definition:");
    note_text.push_back(format.format(
        "{} * {}:",
        FormatParam{num_pals_in_primary.name(), Style::bold | Style::yellow},
        FormatParam{"nontransparent_colors_per_pal", Style::bold}));
    note_text.push_back(format.format(
        "{} * {} = {}",
        FormatParam{num_pals_in_primary.value(), Style::bold | Style::yellow},
        FormatParam{(pal::max_size - 1), Style::bold},
        FormatParam{color_count_limit, Style::bold}));
    note_text.emplace_back("");
    std::ranges::copy(num_pals_in_primary.prettify(format), std::back_inserter(note_text));
    return note_text;
}

} // namespace porytiles2
