#include "porytiles/infra/services/ascii_tile_printer.hpp"

#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles;

void push_to_stream(std::stringstream &ss, const std::string_view s, const std::size_t n)
{
    for (std::size_t i = 0; i < n; i++) {
        ss << s;
    }
}

void push_to_stream(
    std::stringstream &ss, const TextFormatter &format, const std::string_view s, Style style, const std::size_t n)
{
    for (std::size_t i = 0; i < n; i++) {
        ss << format.format("{}", FormatParam{s, style});
    }
}

void reset_stream(std::stringstream &ss)
{
    ss.clear();
    ss.str(std::string{});
}

Style rgba_to_fg_style(const Rgba32 &color)
{
    return rgb_fg_style(color.red(), color.green(), color.blue());
}

Style rgba_to_bg_style(const Rgba32 &color)
{
    return rgb_bg_style(color.red(), color.green(), color.blue());
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
    const PixelTile<Rgba32> &tile,
    const std::set<std::pair<std::size_t, std::size_t>> &highlight_coords,
    const Rgba32 &extrinsic_transparency,
    const TextFormatter *format)
{
    std::vector<std::string> result{};
    std::stringstream ss{};

    // Insert a blank line
    result.emplace_back();

    for (std::size_t row = 0; row < tile::side_length_pix; row++) {
        for (std::size_t col = 0; col < tile::side_length_pix; col++) {
            auto pixel_color = tile.at(row, col);
            if (pixel_color.is_intrinsically_transparent()) {
                pixel_color = extrinsic_transparency;
            }
            const auto color_style_bg = rgba_to_bg_style(pixel_color);
            const auto color_style_fg = rgba_to_fg_style(pixel_color);
            if (highlight_coords.contains({row, col})) {
                ss << format->format("{}", FormatParam{"XX", Style::bold | Style::blink | color_style_fg});
            }
            else {
                ss << format->format("{}", FormatParam{"  ", Style::bold | color_style_bg});
            }
        }
        result.push_back(ss.str());
        reset_stream(ss);
    }

    // Insert a blank line
    result.emplace_back();

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
const PixelTile<Rgba32> &
get_tile_from_metatile(const Metatile<Rgba32> &metatile, metatile::Layer layer, metatile::Subtile subtile)
{
    const auto subtile_index = static_cast<std::size_t>(subtile);
    switch (layer) {
    case metatile::Layer::bottom:
        return metatile.bottom(subtile_index);
    case metatile::Layer::middle:
        return metatile.middle(subtile_index);
    case metatile::Layer::top:
        return metatile.top(subtile_index);
    }
    panic("invalid layer value");
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
 * @param highlight_subtile Toggle to highlight the provided subtile
 * @return A vector of strings representing the rendered metatile
 */
std::vector<std::string> render_metatile_with_highlights(
    const Metatile<Rgba32> &metatile,
    metatile::Layer layer,
    metatile::Subtile subtile,
    const std::set<std::pair<std::size_t, std::size_t>> &highlight_coords,
    const Rgba32 &extrinsic_transparency,
    const TextFormatter *format,
    bool highlight_subtile)
{
    std::vector<std::string> result{};
    std::stringstream ss{};

    // Insert a blank line
    result.emplace_back();

    if (highlight_subtile && subtile == metatile::Subtile::northwest) {
        push_to_stream(ss, *format, "↓", Style::bold | Style::yellow, 16);
        result.push_back(ss.str());
        reset_stream(ss);
    }
    if (highlight_subtile && subtile == metatile::Subtile::northeast) {
        push_to_stream(ss, " ", 16 + 1); // +1 to account for center space
        push_to_stream(ss, *format, "↓", Style::bold | Style::yellow, 16);
        result.push_back(ss.str());
        reset_stream(ss);
    }

    for (std::size_t i = 0; i < metatile::side_length_pix; i++) {
        for (std::size_t j = 0; j < metatile::side_length_pix; j++) {
            // Determine which subtile (i, j) is in and compute subtile-local coordinates
            metatile::Subtile current_subtile{};
            std::size_t subtile_row = 0;
            std::size_t subtile_col = 0;

            if (i < 8 && j < 8) {
                current_subtile = metatile::Subtile::northwest;
                subtile_row = i;
                subtile_col = j;
            }
            else if (i < 8 && j >= 8) {
                current_subtile = metatile::Subtile::northeast;
                subtile_row = i;
                subtile_col = j - 8;
            }
            else if (i >= 8 && j < 8) {
                current_subtile = metatile::Subtile::southwest;
                subtile_row = i - 8;
                subtile_col = j;
            }
            else {
                current_subtile = metatile::Subtile::southeast;
                subtile_row = i - 8;
                subtile_col = j - 8;
            }

            const bool is_in_target_subtile = current_subtile == subtile;
            const auto &current_tile = get_tile_from_metatile(metatile, layer, current_subtile);
            auto pixel_color = current_tile.at(subtile_row, subtile_col);
            if (pixel_color.is_intrinsically_transparent()) {
                pixel_color = extrinsic_transparency;
            }
            const auto color_style_bg = rgba_to_bg_style(pixel_color);
            const auto color_style_fg = rgba_to_fg_style(pixel_color);

            if (is_in_target_subtile) {
                // In target subtile: show pixel highlight if requested, highlight subtile if requested
                if (highlight_coords.contains({subtile_row, subtile_col})) {
                    ss << format->format("{}", FormatParam{"XX", Style::bold | Style::blink | color_style_fg});
                }
                else {
                    ss << format->format("{}", FormatParam{"  ", Style::bold | color_style_bg});
                }
            }
            else {
                // In non-target subtile: show styled blank with the pixel's RGB color (non-bold)
                const auto styled_star = format->format("{}", FormatParam{"  ", color_style_bg});
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

    if (highlight_subtile && subtile == metatile::Subtile::southwest) {
        push_to_stream(ss, *format, "↑", Style::bold | Style::yellow, 16);
        result.push_back(ss.str());
        reset_stream(ss);
    }
    if (highlight_subtile && subtile == metatile::Subtile::southeast) {
        push_to_stream(ss, " ", 16 + 1); // +1 to account for center space
        push_to_stream(ss, *format, "↑", Style::bold | Style::yellow, 16);
        result.push_back(ss.str());
        reset_stream(ss);
    }

    // Insert a blank line
    result.emplace_back();

    return result;
}

} // namespace

namespace porytiles {

std::vector<std::string> AsciiTilePrinter::print_metatile(
    const Metatile<Rgba32> &metatile, metatile::Layer layer, const Rgba32 &extrinsic_transparency) const
{
    return render_metatile_with_highlights(
        metatile, layer, metatile::Subtile::northeast, {}, extrinsic_transparency, format_, false);
}

std::vector<std::string> AsciiTilePrinter::print_metatile_tile_highlight(
    const Metatile<Rgba32> &metatile,
    metatile::Layer layer,
    metatile::Subtile subtile,
    const Rgba32 &extrinsic_transparency) const
{
    return render_metatile_with_highlights(metatile, layer, subtile, {}, extrinsic_transparency, format_, true);
}

std::vector<std::string> AsciiTilePrinter::print_metatile_pixel_highlight(
    const Metatile<Rgba32> &metatile,
    metatile::Layer layer,
    metatile::Subtile subtile,
    std::size_t row,
    std::size_t col,
    const Rgba32 &extrinsic_transparency) const
{
    std::set<std::pair<std::size_t, std::size_t>> coords{{row, col}};
    return render_metatile_with_highlights(metatile, layer, subtile, coords, extrinsic_transparency, format_, true);
}

std::vector<std::string> AsciiTilePrinter::print_metatile_pixel_highlights(
    const Metatile<Rgba32> &metatile,
    metatile::Layer layer,
    metatile::Subtile subtile,
    const std::vector<std::size_t> &indexes,
    const Rgba32 &extrinsic_transparency) const
{
    std::set<std::pair<std::size_t, std::size_t>> coords{};
    for (const auto index : indexes) {
        coords.insert(tile::index_to_row_col(index));
    }
    return render_metatile_with_highlights(metatile, layer, subtile, coords, extrinsic_transparency, format_, true);
}

std::vector<std::string>
AsciiTilePrinter::print_tile(const PixelTile<Rgba32> &tile, const Rgba32 &extrinsic_transparency) const
{
    return render_tile_with_highlights(tile, {}, extrinsic_transparency, format_);
}

std::vector<std::string>
AsciiTilePrinter::print_tile(const PixelTile<IndexPixel> &tile, const Rgba32 &extrinsic_transparency) const
{
    const auto greyscale_pal = standard_greyscale_pal();

    std::vector<std::string> result{};
    std::stringstream ss{};

    // Insert a blank line
    result.emplace_back();

    for (std::size_t row = 0; row < tile::side_length_pix; row++) {
        for (std::size_t col = 0; col < tile::side_length_pix; col++) {
            const auto index_pixel = tile.at(row, col);
            // Use color_index() to extract the lower 4 bits, which is always in range [0, 15].
            // This handles both standard 4-bit pixels and true-color encoded 8-bit pixels correctly.
            const Rgba32 pixel_color = greyscale_pal[index_pixel.color_index()];
            const auto color_style_bg = rgba_to_bg_style(pixel_color);
            ss << format_->format("{}", FormatParam{"  ", Style::bold | color_style_bg});
        }
        result.push_back(ss.str());
        reset_stream(ss);
    }

    // Insert a blank line
    result.emplace_back();

    return result;
}

std::vector<std::string> AsciiTilePrinter::print_tile_pixel_highlight(
    const PixelTile<Rgba32> &tile, std::size_t row, std::size_t col, const Rgba32 &extrinsic_transparency) const
{
    std::set<std::pair<std::size_t, std::size_t>> coords{{row, col}};
    return render_tile_with_highlights(tile, coords, extrinsic_transparency, format_);
}

std::vector<std::string> AsciiTilePrinter::print_tile_pixel_highlights(
    const PixelTile<Rgba32> &tile, const std::vector<std::size_t> &indexes, const Rgba32 &extrinsic_transparency) const
{
    std::set<std::pair<std::size_t, std::size_t>> coords{};
    for (const auto index : indexes) {
        coords.insert(tile::index_to_row_col(index));
    }
    return render_tile_with_highlights(tile, coords, extrinsic_transparency, format_);
}

} // namespace porytiles
