#include "porytiles2/domain/services/tile_validator.hpp"

#include <unordered_set>

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace {

void report_validation_error(
    const porytiles2::Metatile<porytiles2::Rgba32> &metatile,
    std::size_t metatile_index,
    std::size_t internal_tile_index,
    std::size_t row,
    std::size_t col,
    const std::string &diagnostic_code,
    const std::string &error_message,
    const porytiles2::TextFormatter *format,
    const porytiles2::UserDiagnostics *diag,
    const porytiles2::TilePrinter *tile_printer)
{
    auto [layer, subtile] = porytiles2::metatile::from_internal_tile_index(internal_tile_index);
    std::vector errors = {format->format(
        "{}: {}",
        porytiles2::FormatParam{
            porytiles2::metatile::message_header(metatile_index, layer, subtile, row, col, *format)},
        porytiles2::FormatParam{error_message})};
    errors.emplace_back("");
    std::vector highlight = tile_printer->print_metatile_highlight(metatile, layer, subtile, row, col);
    std::ranges::copy(highlight, std::back_inserter(errors));
    diag->err(diagnostic_code, errors);
}

} // namespace

namespace porytiles2 {

ChainableResult<void> TileValidator::validate_alpha_channels(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    bool hit_error = false;
    std::size_t metatile_index = 0;
    for (const auto &metatile : metatiles) {
        const auto tiles = metatile.decompose();

        // Iterate over each internal tile
        for (std::size_t internal_tile_index = 0; internal_tile_index < tiles.size(); ++internal_tile_index) {
            const auto &tile = tiles[internal_tile_index];

            // Iterate over each pixel in the current internal tile
            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    const auto &pixel = tile.at(row, col);
                    if (pixel.alpha() != Rgba32::alpha_opaque && pixel.alpha() != Rgba32::alpha_transparent) {
                        hit_error = true;
                        std::string error_message = format_->format(
                            "invalid alpha channel: {}", FormatParam{std::to_string(pixel.alpha()), Style::bold});
                        report_validation_error(
                            metatile,
                            metatile_index,
                            internal_tile_index,
                            row,
                            col,
                            "alpha-channel-validation",
                            error_message,
                            format_,
                            diag_,
                            tile_printer_);
                    }
                }
            }
        }
        metatile_index++;
    }

    if (hit_error) {
        return FormattableError{"alpha channel violation: found invalid alpha channels"};
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

        // Iterate over each internal tile
        for (std::size_t internal_tile_index = 0; internal_tile_index < tiles.size(); ++internal_tile_index) {
            const auto &tile = tiles[internal_tile_index];
            std::unordered_set<Rgba32> unique_colors;

            // Iterate over each pixel in the current internal tile
            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    const auto &pixel = tile.at(row, col);
                    if (pixel.alpha() != Rgba32::alpha_transparent && pixel != extrinsic) {
                        unique_colors.insert(pixel);
                    }

                    if (unique_colors.size() > pal::max_size - 1) {
                        hit_error = true;
                        std::string error_message = format_->format(
                            "found {}th unique color: {}",
                            FormatParam{pal::max_size},
                            FormatParam{pixel.to_jasc_str(), Style::bold});
                        report_validation_error(
                            metatile,
                            metatile_index,
                            internal_tile_index,
                            row,
                            col,
                            "color-count-validation",
                            error_message,
                            format_,
                            diag_,
                            tile_printer_);
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
            "color constraint violation: found tile(s) with more than {} unique non-transparent pixels",
            FormatParam{pal::max_size - 1}};
    }

    return {};
}

ChainableResult<void>
TileValidator::generate_precision_loss_warnings(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    // TODO: implement
    (void)metatiles.size();
    // When implemented, decompose metatiles: std::vector<PixelTile<Rgba32>> tiles = metatile::decompose(metatiles);
    return {};
}

} // namespace porytiles2
