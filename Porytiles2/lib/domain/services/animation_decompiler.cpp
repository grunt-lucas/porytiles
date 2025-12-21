#include "porytiles2/domain/services/animation_decompiler.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <string>

#include "porytiles2/domain/models/animation_frame.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Finds the palette index for animation tiles by scanning metatile entries.
 *
 * @details
 * Scans all metatile entries to find which palette indices are used when referencing tiles in the animation's
 * tile range (from tile_offset to tile_offset + tile_count - 1). All animation tiles must use the same palette.
 *
 * @param anim_name The name of the anim
 * @param tile_offset Starting tile index for the animation
 * @param tile_count Number of tiles in the animation
 * @param metatiles_bin The metatile entries to scan
 * @pre tile_count must be greater than 0
 * @return The palette index used by all animation tiles
 */
std::size_t find_palette_for_animation_tiles(
    const std::string &anim_name,
    std::size_t tile_offset,
    std::size_t tile_count,
    std::span<const TilemapEntry> metatiles_bin)
{
    std::set<std::size_t> found_pal_indices{};

    // Scan all tiles in the animation range
    for (std::size_t i = 0; i < tile_count; ++i) {
        const std::size_t tile_index = tile_offset + i;
        for (const auto &entry : metatiles_bin) {
            if (entry.tile_index() == tile_index) {
                found_pal_indices.insert(entry.pal_index());
            }
        }
    }

    if (found_pal_indices.empty()) {
        /*
         * TODO: some tilesets will hit this case. E.g. land_water_edge in vanilla primary general is not used within
         * general itself. But the tiles within the anim range are referenced in lilycove tileset, using primary pal 3.
         * So maybe here we should warn the user. And perhaps we need to provide some way to allow the user to select
         * which pal to use when importing an animation, if no pal is found? Alternatively, we could scan all the other
         * tilesets in the game and look for context? To do this, we'd need to parse the layouts file to figure out
         * candidates. At a certain point, it becomes ridiculous to try to automate stuff like this. Users will be
         * expected to have some understanding of how the game works in order to use the tool properly.
         */
        std::cerr << "no palette index found for animation '" << anim_name
                  << "' tiles in range [" + std::to_string(tile_offset) + ", " +
                         std::to_string(tile_offset + tile_count - 1) + "]"
                  << std::endl;
        std::cerr << "falling back to pal 0" << std::endl;
        return 0;
        // panic(
        //     "no palette index found for animation tiles in range [" + std::to_string(tile_offset) + ", " +
        //     std::to_string(tile_offset + tile_count - 1) + "]");
    }

    if (found_pal_indices.size() > 1) {
        std::string pal_list;
        for (const auto &pal_idx : found_pal_indices) {
            if (!pal_list.empty()) {
                pal_list += ", ";
            }
            pal_list += std::to_string(pal_idx);
        }
        panic(
            "animation tiles in range [" + std::to_string(tile_offset) + ", " +
            std::to_string(tile_offset + tile_count - 1) + "] use multiple palette indices: " + pal_list);
    }

    return *found_pal_indices.begin();
}

/**
 * @brief Decompiles a single IndexPixel tile to Rgba32 format.
 *
 * @details
 * Converts a single tile from IndexPixel to Rgba32 using the specified palette.
 *
 * @param tile The indexed tile to decompile
 * @param pal The palette to use for color lookup
 * @param extrinsic_transparency The RGBA color representing transparency
 * @return The decompiled RGBA tile
 */
PixelTile<Rgba32> decompile_tile(
    const PixelTile<IndexPixel> &tile, const Palette<Rgba32, pal::max_size> &pal, const Rgba32 &extrinsic_transparency)
{
    PixelTile<Rgba32> result;

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        const IndexPixel &index_pixel = tile.at(i);

        if (index_pixel.is_transparent()) {
            // TODO: this should be configurable and set by the tileset.import.transparency config
            result.set(i, extrinsic_transparency);
        }
        else {
            const std::size_t pal_index = index_pixel.index();
            if (pal_index >= pal::max_size) {
                panic("palette index out of bounds: " + std::to_string(pal_index));
            }
            result.set(i, pal.at(pal_index));
        }
    }

    return result;
}

/*
 * TODO: this function should be used somewhere?
 */
/**
 * @brief Extracts animation tiles from a tiles.png image.
 *
 * @details
 * Extracts tiles from the specified offset in tiles.png for the given tile count. This is used to get the keyframe
 * tiles for an animation given its parameters.
 *
 * @param tiles_png The tiles.png image (indexed format)
 * @param tile_offset Starting tile index in tiles.png
 * @param tile_count Number of tiles to extract
 * @return Vector of extracted tiles
 */
std::vector<PixelTile<IndexPixel>>
extract_animation_tiles(const Image<IndexPixel> &tiles_png, std::size_t tile_offset, std::size_t tile_count)
{
    std::vector<PixelTile<IndexPixel>> result;
    result.reserve(tile_count);

    // tiles.png is 128 pixels wide (16 tiles per row)
    constexpr std::size_t tiles_per_row = 16;

    const std::size_t img_height = tiles_png.height();
    const std::size_t total_tiles = (tiles_png.width() / tile::side_length_pix) * (img_height / tile::side_length_pix);

    if (tile_offset + tile_count > total_tiles) {
        panic("tile_offset + tile_count exceeds tiles in tiles.png");
    }

    for (std::size_t i = 0; i < tile_count; ++i) {
        const std::size_t tile_idx = tile_offset + i;
        const std::size_t tile_row = tile_idx / tiles_per_row;
        const std::size_t tile_col = tile_idx % tiles_per_row;

        const std::size_t pixel_x = tile_col * tile::side_length_pix;
        const std::size_t pixel_y = tile_row * tile::side_length_pix;

        PixelTile<IndexPixel> tile;
        for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
            for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                tile.set(row, col, tiles_png.at(pixel_y + row, pixel_x + col));
            }
        }

        result.push_back(std::move(tile));
    }

    return result;
}

} // namespace

namespace porytiles2 {

Animation<Rgba32> AnimationDecompiler::decompile_animation(
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
    std::span<const TilemapEntry> metatiles_bin,
    const Rgba32 &extrinsic_transparency) const
{
    Animation<Rgba32> result{anim.name()};
    result.params(anim.params());

    // Get the tile offset from animation params to determine which tile index to look for in metatiles
    const std::size_t tile_offset = anim.params().tile_offset();
    const std::size_t tile_count = anim.params().tile_count();

    // Recover the palette index by scanning metatiles for all animation tiles
    // All tiles in an animation must use the same palette
    const std::size_t pal_index = find_palette_for_animation_tiles(anim.name(), tile_offset, tile_count, metatiles_bin);

    const auto &pal = pals.at(pal_index);

    for (const auto &frame : anim.frames()) {
        std::vector<PixelTile<Rgba32>> rgba_tiles;
        rgba_tiles.reserve(frame.tiles().size());

        for (const auto &index_tile : frame.tiles()) {
            rgba_tiles.push_back(decompile_tile(index_tile, pal, extrinsic_transparency));
        }

        AnimationFrame rgba_frame{frame.frame_name(), std::move(rgba_tiles)};
        result.add_frame(std::move(rgba_frame));
    }

    return result;
}

} // namespace porytiles2
