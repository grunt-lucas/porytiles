#include "porytiles2/domain/services/animation_decompiler.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <string>

#include "porytiles2/domain/algorithms/tile_converters.hpp"
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
         *
         * As a fallback, after performing a full tileset scan, if still no usages were found, we could just check the
         * Porymap pals and see if our key-frame subtile (when interpreted as an Rgba PixelTile via the internal
         * palette) matches any of those pals. If it does, just use that pal here, but still note to the user that this
         * is what happened.
         *
         * Finally, if even that doesn't yield a match, output the tile using greyscale. And warn the user that is
         * what's happening.
         */
        std::cerr << std::endl;
        std::cerr << "---------------------------------" << std::endl;
        std::cerr << "|            TODO               |" << std::endl;
        std::cerr << "---------------------------------" << std::endl;
        std::cerr << "no palette index found for animation '" << anim_name
                  << "' tiles in range [" + std::to_string(tile_offset) + ", " +
                         std::to_string(tile_offset + tile_count) + ")"
                  << std::endl;
        std::cerr << "falling back to pal 0" << std::endl;
        return 0;
    }

    if (found_pal_indices.size() > 1) {
        /*
         * TODO: handle this without panicking
         */
        std::string pal_list;
        for (const auto &pal_idx : found_pal_indices) {
            if (!pal_list.empty()) {
                pal_list += ", ";
            }
            pal_list += std::to_string(pal_idx);
        }
        panic(
            "animation tiles in range [" + std::to_string(tile_offset) + ", " +
            std::to_string(tile_offset + (tile_count - 1)) + "] use multiple palette indices: " + pal_list);
    }

    return *found_pal_indices.begin();
}

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
    const Image<IndexPixel> &tiles_png,
    const Rgba32 &extrinsic_transparency) const
{
    Animation<Rgba32> result{anim.name()};
    result.params(anim.params());

    // Get the tile offset from animation params to determine which tile index to look for in metatiles
    const std::size_t tile_offset = anim.params().tile_offset();
    const std::size_t tile_count = anim.params().tile_count();

    /*
     * TODO: adapt this code so that it computes a separate pal index for each subtile of the key frame. Technically,
     * advanced users could make animations where different subtiles use different palettes. None of the vanilla game
     * animations work this way, but it's possible and thus a use-case I want to support.
     */
    // Recover the palette index by scanning metatiles for all animation tiles
    // All tiles in an animation must use the same palette
    const std::size_t pal_index = find_palette_for_animation_tiles(anim.name(), tile_offset, tile_count, metatiles_bin);

    const auto &pal = pals.at(pal_index);

    /*
     * TODO: this is broken until we implement the anim param reading. Currently, when importing a tileset, tile_offset
     * and tile_count come back as default zero values, since there is no param parsing. So this vector is empty and
     * thus the saved key frame will be an empty file.
     */
    // Extract key frame tiles from tiles.png
    std::vector<PixelTile<IndexPixel>> key_frame_index_tiles =
        extract_animation_tiles(tiles_png, tile_offset, tile_count);

    // Check for duplicate tiles within the key frame
    for (std::size_t i = 0; i < key_frame_index_tiles.size(); ++i) {
        for (std::size_t j = i + 1; j < key_frame_index_tiles.size(); ++j) {
            if (key_frame_index_tiles[i] == key_frame_index_tiles[j]) {
                std::cerr << std::endl;
                std::cerr << "---------------------------------" << std::endl;
                std::cerr << "|            TODO               |" << std::endl;
                std::cerr << "---------------------------------" << std::endl;
                std::cerr << "Animation '" << anim.name() << "' has duplicate key frame tiles at indices:" << std::endl;
                std::cerr << " - " << std::to_string(i) << std::endl;
                std::cerr << " - " << std::to_string(j) << std::endl;
            }
        }
    }

    // Decompile key frame tiles to Rgba32
    std::vector<PixelTile<Rgba32>> key_frame_rgba_tiles;
    key_frame_rgba_tiles.reserve(key_frame_index_tiles.size());
    for (const auto &index_tile : key_frame_index_tiles) {
        key_frame_rgba_tiles.push_back(color_tile_from_index_tile(index_tile, pal, extrinsic_transparency));
    }

    // Set the key frame on the result
    AnimationFrame key_frame{"key", std::move(key_frame_rgba_tiles)};
    result.key_frame(std::move(key_frame));

    for (const auto &frame : anim.frames_values()) {
        std::vector<PixelTile<Rgba32>> rgba_tiles;
        rgba_tiles.reserve(frame.tiles().size());

        for (const auto &index_tile : frame.tiles()) {
            rgba_tiles.push_back(color_tile_from_index_tile(index_tile, pal, extrinsic_transparency));
        }

        AnimationFrame rgba_frame{frame.frame_name(), std::move(rgba_tiles)};
        result.put_frame(frame.frame_name(), std::move(rgba_frame));
    }

    return result;
}

} // namespace porytiles2
