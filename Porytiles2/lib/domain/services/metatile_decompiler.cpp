#include "porytiles2/domain/services/metatile_decompiler.hpp"

#include <memory>
#include <vector>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/services/image_tileizer.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

namespace {

/**
 * @brief Converts an IndexPixel tile to an Rgba32 tile using a palette.
 *
 * @details
 * Takes each pixel's index from the IndexPixel tile and looks up the corresponding Rgba32 color from the specified
 * palette to construct a new PixelTile<Rgba32>.
 *
 * @param index_tile The source tile with indexed color pixels
 * @param pal_index The palette index to use for color lookup
 * @param pals The array of palettes containing the color data
 * @return A new PixelTile<Rgba32> with colors from the palette
 */
PixelTile<Rgba32> convert_tile(
    const PixelTile<IndexPixel> &index_tile,
    unsigned int pal_index,
    const std::array<Palette<Rgba32>, pal::num_pals> &pals)
{
    std::array<Rgba32, tile::size_pix> rgba_pixels{};
    const auto &palette_colors = pals[pal_index].colors();

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        const unsigned int color_index = index_tile.at(i).index();
        // TODO: we should normalize transparency here based on user extrinsic transparency setting
        rgba_pixels[i] = palette_colors[color_index];
    }

    return PixelTile{rgba_pixels};
}

} // namespace

ChainableResult<std::vector<Metatile<Rgba32>>> MetatileDecompiler::decompile_metatiles(
    const std::vector<TilemapEntry> &entries,
    const Image<IndexPixel> &tiles_png,
    const std::array<Palette<Rgba32>, pal::num_pals> &pals)
{
    std::vector<Metatile<Rgba32>> decompiled;

    // Precondition: entry vector must be triple-layerized
    if (entries.size() % metatile::entries_per_metatile_triple != 0) {
        panic("entry vector size was not divisible 12");
    }

    ImageTileizer<IndexPixel> tileizer{};
    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles, tileizer.tileize(tiles_png), "failed to tileize tiles.png", std::vector<Metatile<Rgba32>>);

    // Process metatiles in groups of 12 entries
    for (std::size_t metatile_idx = 0; metatile_idx < entries.size() / metatile::entries_per_metatile_triple;
         ++metatile_idx) {
        Metatile<Rgba32> metatile{};
        const std::size_t base_entry_idx = metatile_idx * metatile::entries_per_metatile_triple;

        // Process bottom layer (entries 0-3)
        for (std::size_t i = 0; i < metatile::tiles_per_metatile_layer; ++i) {
            const auto &entry = entries[base_entry_idx + i];
            const auto &index_tile = tiles[entry.tile_index()];
            auto flipped_tile = index_tile.flip(entry.hflip(), entry.vflip());
            auto rgba_tile = convert_tile(flipped_tile, entry.pal_index(), pals);
            metatile.set_bottom(i, std::move(rgba_tile));
        }

        // Process middle layer (entries 4-7)
        for (std::size_t i = 0; i < metatile::tiles_per_metatile_layer; ++i) {
            const auto &entry = entries[base_entry_idx + metatile::tiles_per_metatile_layer + i];
            const auto &index_tile = tiles[entry.tile_index()];
            auto flipped_tile = index_tile.flip(entry.hflip(), entry.vflip());
            auto rgba_tile = convert_tile(flipped_tile, entry.pal_index(), pals);
            metatile.set_middle(i, std::move(rgba_tile));
        }

        // Process top layer (entries 8-11)
        for (std::size_t i = 0; i < metatile::tiles_per_metatile_layer; ++i) {
            const auto &entry = entries[base_entry_idx + 2 * metatile::tiles_per_metatile_layer + i];
            const auto &index_tile = tiles[entry.tile_index()];
            auto flipped_tile = index_tile.flip(entry.hflip(), entry.vflip());
            auto rgba_tile = convert_tile(flipped_tile, entry.pal_index(), pals);
            metatile.set_top(i, std::move(rgba_tile));
        }

        decompiled.push_back(std::move(metatile));
    }

    return decompiled;
}

} // namespace porytiles2
