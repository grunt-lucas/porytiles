#include "porytiles2/domain/services/tile_validator.hpp"

#include <unordered_set>

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba_tile.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<void> TileValidator::validate_alpha_channels(const std::vector<RgbaTile> &tiles) const
{
    bool hit_error = false;
    std::size_t tile_index = 0;
    for (const auto &tile : tiles) {
        for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
            for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                const auto &pixel = tile.at(row, col);
                if (pixel.alpha() != Rgba32::alpha_opaque && pixel.alpha() != Rgba32::alpha_transparent) {
                    // if (pixel == Rgba32{65, 90, 189}) {
                    hit_error = true;
                    auto [metatile_index, layer, subtile] = metatile::from_tile_index(tile_index);
                    std::vector errors = {format_->format(
                        "{}: invalid alpha channel: {}",
                        FormatParam{metatile::message_header(metatile_index, layer, subtile, row, col, *format_)},
                        FormatParam{std::to_string(pixel.alpha()), Style::bold})};
                    std::vector highlight = tile_printer_->print_metatile_highlight(subtile, row, col, Style::red);
                    std::ranges::copy(highlight, std::back_inserter(errors));
                    diag_->err(errors);
                }
            }
        }
        tile_index++;
    }

    if (hit_error) {
        return FormattableError{"alpha channel validation failed"};
    }

    return {};
}

ChainableResult<void>
TileValidator::validate_unique_color_count(const std::vector<RgbaTile> &tiles, const Rgba32 &extrinsic) const
{
    bool hit_error = false;
    std::size_t tile_index = 0;
    for (const auto &tile : tiles) {
        std::unordered_set<Rgba32> unique_colors;
        for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
            for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                const auto &pixel = tile.at(row, col);
                if (pixel.alpha() != Rgba32::alpha_transparent && pixel != extrinsic) {
                    unique_colors.insert(pixel);
                }
                // TODO: don't hardcode 15 here
                if (unique_colors.size() > 15) {
                    // if (unique_colors.size() > 7) {
                    hit_error = true;
                    auto [metatile_index, layer, subtile] = metatile::from_tile_index(tile_index);
                    // TODO: don't hardcode 16 here
                    std::vector errors = {format_->format(
                        "{}: found 16th unique color: {}",
                        FormatParam{metatile::message_header(metatile_index, layer, subtile, row, col, *format_)},
                        FormatParam{pixel.to_jasc_str(), Style::bold})};
                    std::vector highlight = tile_printer_->print_metatile_highlight(subtile, row, col, Style::red);
                    std::ranges::copy(highlight, std::back_inserter(errors));
                    diag_->err(errors);
                    goto next_tile;
                }
            }
        }
    next_tile:
        tile_index++;
    }

    if (hit_error) {
        return FormattableError{
            "unique color constraint violation: tile had more than 15 unique non-transparent pixels"};
    }

    return {};
}

ChainableResult<void> TileValidator::generate_precision_loss_warnings(const std::vector<RgbaTile> &tiles) const
{
    // TODO: implement
    return {};
}

} // namespace porytiles2
