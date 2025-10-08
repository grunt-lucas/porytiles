#include "porytiles2/domain/services/rgba_layer_image_metatileizer.hpp"

#include <cstddef>

#include "fmt/format.h"

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/rgba_metatile.hpp"
#include "porytiles2/domain/models/rgba_tile.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

namespace {

void populate_metatile_at_position(
    RgbaMetatile &metatile,
    const std::vector<RgbaTile> &bottom_tiles,
    const std::vector<RgbaTile> &middle_tiles,
    const std::vector<RgbaTile> &top_tiles,
    std::size_t metatile_row,
    std::size_t metatile_col,
    std::size_t tiles_per_image_row)
{
    for (std::size_t tile_idx = 0; tile_idx < metatile::tiles_per_metatile_layer; ++tile_idx) {
        // Calculate tile position within the metatile
        const std::size_t tile_row = tile_idx / metatile::tiles_per_side;
        const std::size_t tile_col = tile_idx % metatile::tiles_per_side;

        // Calculate which tile index we need from the tileized arrays
        const std::size_t global_tile_row = metatile_row * metatile::tiles_per_side + tile_row;
        const std::size_t global_tile_col = metatile_col * metatile::tiles_per_side + tile_col;
        const std::size_t global_tile_idx = global_tile_row * tiles_per_image_row + global_tile_col;

        // Set tiles in the metatile from the tileized arrays
        metatile.set_bottom(tile_idx, bottom_tiles[global_tile_idx]);
        metatile.set_middle(tile_idx, middle_tiles[global_tile_idx]);
        metatile.set_top(tile_idx, top_tiles[global_tile_idx]);
    }
}

void copy_metatile_to_images(
    const RgbaMetatile &metatile,
    Image<Rgba32> &bottom_image,
    Image<Rgba32> &middle_image,
    Image<Rgba32> &top_image,
    std::size_t metatile_row,
    std::size_t metatile_col)
{
    // Extract tiles from this metatile and place them in the appropriate image positions
    for (std::size_t tile_idx = 0; tile_idx < metatile::tiles_per_metatile_layer; ++tile_idx) {
        // Calculate tile position within the metatile
        const std::size_t tile_row = tile_idx / metatile::tiles_per_side;
        const std::size_t tile_col = tile_idx % metatile::tiles_per_side;

        // Calculate the starting pixel position for this tile in the image
        const std::size_t start_pixel_row = metatile_row * metatile::side_length_pix + tile_row * tile::side_length_pix;
        const std::size_t start_pixel_col = metatile_col * metatile::side_length_pix + tile_col * tile::side_length_pix;

        // Copy pixels from each layer's tile to the corresponding image
        const auto &bottom_tile = metatile.bottom(tile_idx);
        const auto &middle_tile = metatile.middle(tile_idx);
        const auto &top_tile = metatile.top(tile_idx);

        for (std::size_t pixel_row = 0; pixel_row < tile::side_length_pix; ++pixel_row) {
            for (std::size_t pixel_col = 0; pixel_col < tile::side_length_pix; ++pixel_col) {
                const std::size_t image_row = start_pixel_row + pixel_row;
                const std::size_t image_col = start_pixel_col + pixel_col;

                bottom_image.set(image_row, image_col, bottom_tile.at(pixel_row, pixel_col));
                middle_image.set(image_row, image_col, middle_tile.at(pixel_row, pixel_col));
                top_image.set(image_row, image_col, top_tile.at(pixel_row, pixel_col));
            }
        }
    }
}

void fill_region_with_transparent(
    Image<Rgba32> &bottom_image,
    Image<Rgba32> &middle_image,
    Image<Rgba32> &top_image,
    std::size_t metatile_row,
    std::size_t metatile_col)
{
    const Rgba32 transparent_pixel{0, 0, 0, 0};

    // Fill the entire 16x16 region for this metatile position with transparent pixels
    for (std::size_t pixel_row = 0; pixel_row < metatile::side_length_pix; ++pixel_row) {
        for (std::size_t pixel_col = 0; pixel_col < metatile::side_length_pix; ++pixel_col) {
            const std::size_t image_row = metatile_row * metatile::side_length_pix + pixel_row;
            const std::size_t image_col = metatile_col * metatile::side_length_pix + pixel_col;

            bottom_image.set(image_row, image_col, transparent_pixel);
            middle_image.set(image_row, image_col, transparent_pixel);
            top_image.set(image_row, image_col, transparent_pixel);
        }
    }
}

} // namespace

[[nodiscard]] ChainableResult<std::vector<RgbaMetatile>> RgbaLayerImageMetatileizer::metatileize(
    const Image<Rgba32> &bottom, const Image<Rgba32> &middle, const Image<Rgba32> &top) const
{
    // Validate that all images have the same dimensions
    if (bottom.width() != middle.width() || bottom.height() != middle.height() || bottom.width() != top.width() ||
        bottom.height() != top.height()) {
        return FormattableError{fmt::format(
            "layer images have mismatched dimensions: bottom={}x{}, middle={}x{}, top={}x{}",
            bottom.width(),
            bottom.height(),
            middle.width(),
            middle.height(),
            top.width(),
            top.height())};
    }

    // Tileize each layer image
    const auto bottom_tiles_result = tileizer_.tileize(bottom);
    if (!bottom_tiles_result.has_value()) {
        return ChainableResult<std::vector<RgbaMetatile>>::chain_together(
            FormattableError{"failed to tileize bottom layer"}, bottom_tiles_result);
    }
    const auto &bottom_tiles = bottom_tiles_result.value();

    const auto middle_tiles_result = tileizer_.tileize(middle);
    if (!middle_tiles_result.has_value()) {
        return ChainableResult<std::vector<RgbaMetatile>>::chain_together(
            FormattableError{"failed to tileize middle layer"}, middle_tiles_result);
    }
    const auto &middle_tiles = middle_tiles_result.value();

    const auto top_tiles_result = tileizer_.tileize(top);
    if (!top_tiles_result.has_value()) {
        return ChainableResult<std::vector<RgbaMetatile>>::chain_together(
            FormattableError{"failed to tileize top layer"}, top_tiles_result);
    }
    const auto &top_tiles = top_tiles_result.value();

    /*
     * Validate that dimensions are multiples of metatile size. We already validated that all image dimensions are
     * identical, so we can just check bottom here as a surrogate for the other two layers. Additionally, we already
     * checked in the tileization step if the image dimensions were a multiple of 8. Now, we check that the image
     * dimensions are a multiple of 16 to confirm that it can be correctly metatileized.
     */
    if (bottom.width() % metatile::side_length_pix != 0 || bottom.height() % metatile::side_length_pix != 0) {
        return FormattableError{fmt::format(
            "image dimensions must be multiples of {}, got {}x{}",
            metatile::side_length_pix,
            bottom.width(),
            bottom.height())};
    }

    const std::size_t metatiles_per_row = bottom.width() / metatile::side_length_pix;
    const std::size_t metatiles_per_col = bottom.height() / metatile::side_length_pix;
    const std::size_t total_metatiles = metatiles_per_row * metatiles_per_col;

    std::vector<RgbaMetatile> rgba_metatiles;
    rgba_metatiles.reserve(total_metatiles);

    const std::size_t tiles_per_image_row = bottom.width() / tile::side_length_pix;

    // Process each 16x16 metatile region
    for (std::size_t metatile_row = 0; metatile_row < metatiles_per_col; ++metatile_row) {
        for (std::size_t metatile_col = 0; metatile_col < metatiles_per_row; ++metatile_col) {
            RgbaMetatile metatile;
            populate_metatile_at_position(
                metatile, bottom_tiles, middle_tiles, top_tiles, metatile_row, metatile_col, tiles_per_image_row);
            rgba_metatiles.push_back(std::move(metatile));
        }
    }

    return rgba_metatiles;
}

ChainableResult<std::tuple<Image<Rgba32>, Image<Rgba32>, Image<Rgba32>>> RgbaLayerImageMetatileizer::demetatileize(
    const std::vector<RgbaMetatile> &metatiles, std::size_t metatiles_per_row) const
{
    // Validate input parameters
    if (metatiles_per_row == 0) {
        panic("metatiles_per_row must be greater than zero");
    }

    if (metatiles.empty()) {
        return FormattableError{"input metatiles vector was empty"};
    }

    // Compute metatiles_per_col using ceiling division
    const std::size_t metatiles_per_col = (metatiles.size() + metatiles_per_row - 1) / metatiles_per_row;

    // Calculate image dimensions
    const std::size_t image_width = metatiles_per_row * metatile::side_length_pix;
    const std::size_t image_height = metatiles_per_col * metatile::side_length_pix;

    // Create the three layer images
    Image<Rgba32> bottom_image{image_width, image_height};
    Image<Rgba32> middle_image{image_width, image_height};
    Image<Rgba32> top_image{image_width, image_height};

    // Process each metatile position in the grid
    for (std::size_t metatile_row = 0; metatile_row < metatiles_per_col; ++metatile_row) {
        for (std::size_t metatile_col = 0; metatile_col < metatiles_per_row; ++metatile_col) {
            const std::size_t metatile_idx = metatile_row * metatiles_per_row + metatile_col;

            // Check if we have a metatile for this position, or if we need to pad with empty pixels
            if (metatile_idx < metatiles.size()) {
                const auto &metatile = metatiles[metatile_idx];
                copy_metatile_to_images(metatile, bottom_image, middle_image, top_image, metatile_row, metatile_col);
            }
            else {
                fill_region_with_transparent(bottom_image, middle_image, top_image, metatile_row, metatile_col);
            }
        }
    }

    return std::make_tuple(std::move(bottom_image), std::move(middle_image), std::move(top_image));
}

} // namespace porytiles2
