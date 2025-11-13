#include "porytiles2/domain/services/tile_validator.hpp"

#include <unordered_set>

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<void> TileValidator::validate_alpha_channels(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    bool hit_error = false;
    std::size_t metatile_index = 0;
    for (const auto &metatile : metatiles) {
        const auto tiles = metatile.decompose();
        for (std::size_t internal_tile_index = 0; internal_tile_index < tiles.size(); ++internal_tile_index) {
            const auto &tile = tiles[internal_tile_index];
            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    const auto &pixel = tile.at(row, col);
                    if (pixel.alpha() != Rgba32::alpha_opaque && pixel.alpha() != Rgba32::alpha_transparent) {
                        hit_error = true;
                        auto [layer, subtile] = metatile::from_internal_tile_index(internal_tile_index);
                        std::vector errors = {format_->format(
                            "{}: invalid alpha channel: {}",
                            FormatParam{metatile::message_header(metatile_index, layer, subtile, row, col, *format_)},
                            FormatParam{std::to_string(pixel.alpha()), Style::bold})};
                        errors.emplace_back("");
                        std::vector highlight =
                            tile_printer_->print_metatile_highlight(metatile, layer, subtile, row, col);
                        std::ranges::copy(highlight, std::back_inserter(errors));
                        diag_->err("alpha-channel-validation", errors);
                    }
                }
            }
        }
        metatile_index++;
    }

    if (hit_error) {
        return FormattableError{"alpha channel validation failed"};
    }

    return {};
}

ChainableResult<void> TileValidator::validate_unique_color_count(
    const std::vector<Metatile<Rgba32>> &metatiles, const Rgba32 &extrinsic) const
{
    bool hit_error = false;
    std::size_t metatile_index = 0;
    for (const auto &metatile : metatiles) {
        const auto tiles = metatile.decompose();
        for (std::size_t internal_tile_index = 0; internal_tile_index < tiles.size(); ++internal_tile_index) {
            const auto &tile = tiles[internal_tile_index];
            std::unordered_set<Rgba32> unique_colors;
            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    const auto &pixel = tile.at(row, col);
                    if (pixel.alpha() != Rgba32::alpha_transparent && pixel != extrinsic) {
                        unique_colors.insert(pixel);
                    }

                    if (unique_colors.size() > pal::max_size - 1) {
                        hit_error = true;
                        auto [layer, subtile] = metatile::from_internal_tile_index(internal_tile_index);
                        std::vector errors = {format_->format(
                            "{}: found {}th unique color: {}",
                            FormatParam{metatile::message_header(metatile_index, layer, subtile, row, col, *format_)},
                            FormatParam{pal::max_size},
                            FormatParam{pixel.to_jasc_str(), Style::bold})};
                        errors.emplace_back("");
                        std::vector highlight =
                            tile_printer_->print_metatile_highlight(metatile, layer, subtile, row, col);
                        std::ranges::copy(highlight, std::back_inserter(errors));
                        diag_->err("color-count-validation", errors);
                        goto next_tile;
                    }
                }
            }
        next_tile:;
        }
        metatile_index++;
    }

    if (hit_error) {
        return FormattableError{
            "unique color constraint violation: tile had more than {} unique non-transparent pixels",
            FormatParam{pal::max_size - 1}};
    }

    return {};
}

ChainableResult<void>
TileValidator::generate_precision_loss_warnings(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    // TODO: implement
    // When implemented, decompose metatiles: std::vector<PixelTile<Rgba32>> tiles = metatile::decompose(metatiles);
    return {};
}

} // namespace porytiles2
