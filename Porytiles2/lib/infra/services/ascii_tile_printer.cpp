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

porytiles2::Style rgba_to_fg_style(const porytiles2::Rgba32 &color)
{
    return porytiles2::rgb_fg_style(color.red(), color.green(), color.blue());
}

porytiles2::Style rgba_to_bg_style(const porytiles2::Rgba32 &color)
{
    return porytiles2::rgb_bg_style(color.red(), color.green(), color.blue());
}

/**
 * @brief Helper function to render an 8x8 tile with highlighted pixels.
 *
 * @details
 * This function renders an 8x8 tile grid with highlighted pixels based on the provided set of (row, col) coordinates.
 * Pixels at the highlighted coordinates are marked with "X" and styled with the pixel's actual RGB color. Other pixels
 * are shown as " . ".
 *
 * @param tile The PixelTile to render
 * @param highlight_coords Set of (row, col) coordinates to highlight with "X"
 * @param extrinsic_transparency The extrinsic transparency color to substitute for intrinsically transparent pixels
 * @param format The text formatter to use for styling
 * @return A vector of strings representing the rendered tile
 */
std::vector<std::string> render_tile_with_highlights(
    const porytiles2::PixelTile<porytiles2::Rgba32> &tile,
    const std::set<std::pair<std::size_t, std::size_t>> &highlight_coords,
    const porytiles2::Rgba32 &extrinsic_transparency,
    const porytiles2::TextFormatter *format)
{
    std::vector<std::string> result{};
    std::stringstream ss{};

    for (std::size_t row = 0; row < porytiles2::tile::side_length_pix; row++) {
        for (std::size_t col = 0; col < porytiles2::tile::side_length_pix; col++) {
            auto pixel_color = tile.at(row, col);
            if (pixel_color.is_intrinsically_transparent()) {
                pixel_color = extrinsic_transparency;
            }
            const auto color_style_bg = rgba_to_bg_style(pixel_color);
            const auto color_style_fg = rgba_to_fg_style(pixel_color);
            if (highlight_coords.contains({row, col})) {
                const auto styled_x =
                    format->format("{}", porytiles2::FormatParam{"◢◣", porytiles2::Style::bold | color_style_fg});
                ss << styled_x;
            }
            else {
                const auto styled_star =
                    format->format("{}", porytiles2::FormatParam{"  ", porytiles2::Style::bold | color_style_bg});
                ss << styled_star;
            }
        }
        result.push_back(ss.str());
        reset_stream(ss);
    }

    return result;
}

/**
 * @brief Helper function to extract the correct tile from a metatile.
 *
 * @details
 * Extracts the PixelTile at the specified layer and subtile position within a metatile.
 *
 * @param metatile The metatile to extract from
 * @param layer The layer to extract from (bottom, middle, top)
 * @param subtile The subtile position (northwest, northeast, southwest, southeast)
 * @return Reference to the PixelTile at the specified position
 */
const porytiles2::PixelTile<porytiles2::Rgba32> &get_tile_from_metatile(
    const porytiles2::Metatile<porytiles2::Rgba32> &metatile,
    porytiles2::metatile::Layer layer,
    porytiles2::metatile::Subtile subtile)
{
    const auto subtile_index = static_cast<std::size_t>(subtile);
    switch (layer) {
    case porytiles2::metatile::Layer::bottom:
        return metatile.bottom(subtile_index);
    case porytiles2::metatile::Layer::middle:
        return metatile.middle(subtile_index);
    case porytiles2::metatile::Layer::top:
        return metatile.top(subtile_index);
    }
    porytiles2::panic("invalid layer value");
}

/**
 * @brief Helper function to render a metatile with highlighted pixels.
 *
 * @details
 * This function renders a 16x16 metatile grid with highlighted pixels based on the provided set of (row, col)
 * coordinates. Pixels at the highlighted coordinates are marked with "X" styled with their actual RGB color (bold),
 * other pixels in the target subtile are marked with "*" styled with their actual RGB color (bold), and pixels in
 * non-target subtiles are marked with "." styled with their actual RGB color (non-bold).
 *
 * @param metatile The metatile to render
 * @param layer The layer of the metatile to render
 * @param subtile The subtile being highlighted
 * @param highlight_coords Set of (row, col) coordinates within the subtile to highlight with "X"
 * @param extrinsic_transparency The extrinsic transparency color to substitute for intrinsically transparent pixels
 * @param format The text formatter to use for styling
 * @return A vector of strings representing the rendered metatile
 */
std::vector<std::string> render_metatile_with_highlights(
    const porytiles2::Metatile<porytiles2::Rgba32> &metatile,
    porytiles2::metatile::Layer layer,
    porytiles2::metatile::Subtile subtile,
    const std::set<std::pair<std::size_t, std::size_t>> &highlight_coords,
    const porytiles2::Rgba32 &extrinsic_transparency,
    const porytiles2::TextFormatter *format)
{
    std::vector<std::string> result{};
    std::stringstream ss{};

    for (std::size_t i = 0; i < porytiles2::metatile::side_length_pix; i++) {
        for (std::size_t j = 0; j < porytiles2::metatile::side_length_pix; j++) {
            // Determine which subtile (i, j) is in and compute subtile-local coordinates
            porytiles2::metatile::Subtile current_subtile{};
            std::size_t subtile_row = 0;
            std::size_t subtile_col = 0;

            if (i < 8 && j < 8) {
                current_subtile = porytiles2::metatile::Subtile::northwest;
                subtile_row = i;
                subtile_col = j;
            }
            else if (i < 8 && j >= 8) {
                current_subtile = porytiles2::metatile::Subtile::northeast;
                subtile_row = i;
                subtile_col = j - 8;
            }
            else if (i >= 8 && j < 8) {
                current_subtile = porytiles2::metatile::Subtile::southwest;
                subtile_row = i - 8;
                subtile_col = j;
            }
            else {
                current_subtile = porytiles2::metatile::Subtile::southeast;
                subtile_row = i - 8;
                subtile_col = j - 8;
            }

            const bool is_in_target_subtile = (current_subtile == subtile);
            const auto &current_tile = get_tile_from_metatile(metatile, layer, current_subtile);
            auto pixel_color = current_tile.at(subtile_row, subtile_col);
            if (pixel_color.is_intrinsically_transparent()) {
                pixel_color = extrinsic_transparency;
            }
            const auto color_style_bg = rgba_to_bg_style(pixel_color);
            const auto color_style_fg = rgba_to_fg_style(pixel_color);

            if (is_in_target_subtile) {
                // In target subtile: show X for highlights,blank* for others (both bold)
                if (highlight_coords.contains({subtile_row, subtile_col})) {
                    const auto styled_x =
                        format->format("{}", porytiles2::FormatParam{"◢◣", porytiles2::Style::bold | color_style_fg});
                    ss << styled_x;
                }
                else {
                    const auto styled_star =
                        format->format("{}", porytiles2::FormatParam{"  ", porytiles2::Style::bold | color_style_bg});
                    ss << styled_star;
                }
            }
            else {
                // In non-target subtile: show styled blank with the pixel's RGB color (non-bold)
                const auto styled_star = format->format("{}", porytiles2::FormatParam{"  ", color_style_bg});
                ss << styled_star;
            }

            // If we're at the midpoint cell, add an extra space.
            if (j == 7) {
                ss << " ";
            }

            // Reset once this row is exhausted
            if (j == 15) {
                result.push_back(ss.str());
                reset_stream(ss);
            }
        }

        // Insert a spacer line between top and bottom tiles
        if (i == 7) {
            result.push_back(ss.str());
            reset_stream(ss);
        }
    }

    return result;
}

} // namespace

namespace porytiles2 {

std::vector<std::string> AsciiTilePrinter::print_metatile(
    const Metatile<Rgba32> &metatile, metatile::Layer layer, metatile::Subtile subtile) const
{
    return render_metatile_with_highlights(metatile, layer, subtile, {}, extrinsic_transparency_, format_);
}

std::vector<std::string> AsciiTilePrinter::print_metatile_highlight(
    const Metatile<Rgba32> &metatile,
    metatile::Layer layer,
    metatile::Subtile subtile,
    std::size_t row,
    std::size_t col) const
{
    std::set<std::pair<std::size_t, std::size_t>> coords{{row, col}};
    return render_metatile_with_highlights(metatile, layer, subtile, coords, extrinsic_transparency_, format_);
}

std::vector<std::string> AsciiTilePrinter::print_metatile_highlights(
    const Metatile<Rgba32> &metatile,
    metatile::Layer layer,
    metatile::Subtile subtile,
    const std::vector<std::size_t> &indexes) const
{
    std::set<std::pair<std::size_t, std::size_t>> coords{};
    for (const auto index : indexes) {
        coords.insert(tile::index_to_row_col(index));
    }
    return render_metatile_with_highlights(metatile, layer, subtile, coords, extrinsic_transparency_, format_);
}

std::vector<std::string> AsciiTilePrinter::print_tile(const PixelTile<Rgba32> &tile) const
{
    return render_tile_with_highlights(tile, {}, extrinsic_transparency_, format_);
}

std::vector<std::string>
AsciiTilePrinter::print_tile_highlight(const PixelTile<Rgba32> &tile, std::size_t row, std::size_t col) const
{
    std::set<std::pair<std::size_t, std::size_t>> coords{{row, col}};
    return render_tile_with_highlights(tile, coords, extrinsic_transparency_, format_);
}

std::vector<std::string>
AsciiTilePrinter::print_tile_highlights(const PixelTile<Rgba32> &tile, const std::vector<std::size_t> &indexes) const
{
    std::set<std::pair<std::size_t, std::size_t>> coords{};
    for (const auto index : indexes) {
        coords.insert(tile::index_to_row_col(index));
    }
    return render_tile_with_highlights(tile, coords, extrinsic_transparency_, format_);
}

} // namespace porytiles2
