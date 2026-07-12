#include "porytiles/domain/services/metatile_decompiler.hpp"

#include <memory>
#include <vector>

#include "porytiles/domain/algorithms/tile_converters.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tilemap_entry.hpp"
#include "porytiles/domain/services/image_tileizer.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

ChainableResult<std::vector<Metatile<Rgba32>>> MetatileDecompiler::decompile_metatiles(
    const std::vector<TilemapEntry> &entries,
    const Image<IndexPixel> &tiles_png,
    const std::array<Palette<Rgba32, palette::max_size>, palette::num_palettes> &palettes)
{
    std::vector<Metatile<Rgba32>> decompiled;

    // Precondition: entry vector must be triple-layerized
    if (entries.size() % metatile::entries_per_metatile_triple != 0) {
        panic("entry vector size was not divisible 12");
    }

    ImageTileizer<IndexPixel> tileizer{};
    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles, tileizer.tileize(tiles_png), std::vector<Metatile<Rgba32>>, "Failed to tileize tiles.png.");

    // Process metatiles in groups of 12 entries
    for (std::size_t metatile_idx = 0; metatile_idx < entries.size() / metatile::entries_per_metatile_triple;
         ++metatile_idx) {
        Metatile<Rgba32> metatile{};
        const std::size_t base_entry_idx = metatile_idx * metatile::entries_per_metatile_triple;

        // Process bottom layer (entries 0-3)
        for (std::size_t i = 0; i < metatile::tiles_per_metatile_layer; ++i) {
            const auto &entry = entries[base_entry_idx + i];
            const auto &index_tile = tiles[entry.tile_index()];
            auto flipped_tile = index_tile.flip(entry.h_flip(), entry.v_flip());
            auto rgba_tile =
                color_tile_from_index_tile(flipped_tile, palettes[entry.palette_index()], extrinsic_transparency_);
            metatile.set_bottom(i, std::move(rgba_tile));
        }

        // Process middle layer (entries 4-7)
        for (std::size_t i = 0; i < metatile::tiles_per_metatile_layer; ++i) {
            const auto &entry = entries[base_entry_idx + metatile::tiles_per_metatile_layer + i];
            const auto &index_tile = tiles[entry.tile_index()];
            auto flipped_tile = index_tile.flip(entry.h_flip(), entry.v_flip());
            auto rgba_tile =
                color_tile_from_index_tile(flipped_tile, palettes[entry.palette_index()], extrinsic_transparency_);
            metatile.set_middle(i, std::move(rgba_tile));
        }

        // Process top layer (entries 8-11)
        for (std::size_t i = 0; i < metatile::tiles_per_metatile_layer; ++i) {
            const auto &entry = entries[base_entry_idx + 2 * metatile::tiles_per_metatile_layer + i];
            const auto &index_tile = tiles[entry.tile_index()];
            auto flipped_tile = index_tile.flip(entry.h_flip(), entry.v_flip());
            auto rgba_tile =
                color_tile_from_index_tile(flipped_tile, palettes[entry.palette_index()], extrinsic_transparency_);
            metatile.set_top(i, std::move(rgba_tile));
        }

        decompiled.push_back(std::move(metatile));
    }

    return decompiled;
}

} // namespace porytiles
