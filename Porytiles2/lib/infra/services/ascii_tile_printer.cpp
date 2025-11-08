#include "porytiles2/infra/services/ascii_tile_printer.hpp"

#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace {

void push_to_stream(std::stringstream &ss, const std::string_view s, const std::size_t n)
{
    for (std::size_t i = 0; i < n; i++) {
        ss << s;
    }
}

void reset_stream(std::stringstream &ss)
{
    ss.clear();
    ss.str(std::string{});
}

/**
 * @brief Helper function to render a metatile with highlighted pixels.
 *
 * @details
 * This function renders a 16x16 metatile grid with highlighted pixels based on the provided
 * set of (row, col) coordinates. Pixels at the highlighted coordinates are marked with "X",
 * other pixels in the same subtile are marked with "*", and pixels in other subtiles are
 * marked with "-".
 *
 * @param subtile The subtile being highlighted
 * @param highlight_coords Set of (row, col) coordinates within the subtile to highlight with "X"
 * @param color The color style to apply to the "X" markers
 * @param format The text formatter to use for styling
 * @return A vector of strings representing the rendered metatile
 */
std::vector<std::string> render_metatile_with_highlights(
    porytiles2::metatile::Subtile subtile,
    const std::set<std::pair<std::size_t, std::size_t>> &highlight_coords,
    porytiles2::Style color,
    porytiles2::TextFormatter *format)
{
    std::vector<std::string> highlight{};
    std::stringstream ss{};

    auto styled_x = format->format(" {} ", porytiles2::FormatParam{"X", porytiles2::Style::bold | color});
    auto styled_star = format->format(" {} ", porytiles2::FormatParam{"*", porytiles2::Style::bold});

    for (std::size_t i = 0; i < porytiles2::metatile::side_length_pix; i++) {
        for (std::size_t j = 0; j < porytiles2::metatile::side_length_pix; j++) {
            bool is_in_subtile = false;
            std::size_t subtile_row = 0;
            std::size_t subtile_col = 0;

            // Determine if (i, j) is in the target subtile and compute subtile-local coordinates
            if (subtile == porytiles2::metatile::Subtile::northwest && i < 8 && j < 8) {
                is_in_subtile = true;
                subtile_row = i;
                subtile_col = j;
            }
            else if (subtile == porytiles2::metatile::Subtile::northeast && i < 8 && j >= 8) {
                is_in_subtile = true;
                subtile_row = i;
                subtile_col = j - 8;
            }
            else if (subtile == porytiles2::metatile::Subtile::southwest && i >= 8 && j < 8) {
                is_in_subtile = true;
                subtile_row = i - 8;
                subtile_col = j;
            }
            else if (subtile == porytiles2::metatile::Subtile::southeast && i >= 8 && j >= 8) {
                is_in_subtile = true;
                subtile_row = i - 8;
                subtile_col = j - 8;
            }

            if (is_in_subtile) {
                // Check if this coordinate should be highlighted
                if (highlight_coords.contains({subtile_row, subtile_col})) {
                    ss << format->format("{}", porytiles2::FormatParam{styled_x});
                }
                else {
                    ss << format->format("{}", porytiles2::FormatParam{styled_star});
                }
            }
            else {
                ss << " - ";
            }

            // If we're at the midpoint cell, add an extra space.
            if (j == 7) {
                ss << " ";
            }

            // Reset once this row is exhausted
            if (j == 15) {
                highlight.push_back(ss.str());
                reset_stream(ss);
            }
        }

        // Insert a spacer line between top and bottom tiles
        if (i == 7) {
            highlight.push_back(ss.str());
            reset_stream(ss);
        }
    }

    return highlight;
}

} // namespace

namespace porytiles2 {

std::vector<std::string> AsciiTilePrinter::print_metatile_highlight(
    metatile::Subtile subtile, std::size_t row, std::size_t col, Style color) const
{
    std::set<std::pair<std::size_t, std::size_t>> highlight_coords{};
    highlight_coords.insert({row, col});
    return render_metatile_with_highlights(subtile, highlight_coords, color, format_);
}

std::vector<std::string> AsciiTilePrinter::print_metatile_highlights(
    metatile::Subtile subtile, const std::vector<std::size_t> &indexes, Style color) const
{
    std::set<std::pair<std::size_t, std::size_t>> highlight_coords{};
    for (std::size_t index : indexes) {
        auto [row, col] = tile::index_to_row_col(index);
        highlight_coords.insert({row, col});
    }
    return render_metatile_with_highlights(subtile, highlight_coords, color, format_);
}

} // namespace porytiles2
