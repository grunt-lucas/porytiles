#include "porytiles/domain/services/metatile_decompiler.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "porytiles/domain/algorithms/tile_converters.hpp"
#include "porytiles/domain/config/import_transparency_mode.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tilemap_entry.hpp"
#include "porytiles/domain/services/image_tileizer.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

namespace {

/// @brief Selects the pixel value that represents transparency on one layer of one metatile.
Rgba32 transparent_color_for(
    ImportTransparencyMode mode,
    metatile::Layer layer,
    std::optional<metatile::Layer> synthesized_layer,
    const Rgba32 &extrinsic_transparency)
{
    switch (mode) {
    case ImportTransparencyMode::alpha:
        return Rgba32{};
    case ImportTransparencyMode::extrinsic:
        return extrinsic_transparency;
    case ImportTransparencyMode::mixed:
        return synthesized_layer == layer ? Rgba32{} : extrinsic_transparency;
    }
    panic("unhandled ImportTransparencyMode value in transparent_color_for");
}

} // namespace

ChainableResult<std::vector<Metatile<Rgba32>>> MetatileDecompiler::decompile_metatiles(
    const std::vector<TilemapEntry> &entries,
    const Image<IndexPixel> &tiles_png,
    const std::array<Palette<Rgba32, palette::max_size>, palette::num_palettes> &palettes,
    const ImportTransparencyMode mode,
    const std::vector<std::optional<metatile::Layer>> &synthesized_layers)
{
    std::vector<Metatile<Rgba32>> decompiled;

    // Precondition: entry vector must be triple-layerized
    if (entries.size() % metatile::entries_per_metatile_triple != 0) {
        panic("entry vector size was not divisible 12");
    }
    const std::size_t num_metatiles = entries.size() / metatile::entries_per_metatile_triple;

    // Precondition: synthesized_layers is either absent or has one entry per metatile
    if (!synthesized_layers.empty() && synthesized_layers.size() != num_metatiles) {
        panic(
            "synthesized_layers size " + std::to_string(synthesized_layers.size()) + " != metatile count " +
            std::to_string(num_metatiles));
    }

    ImageTileizer<IndexPixel> tileizer{};
    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles, tileizer.tileize(tiles_png), std::vector<Metatile<Rgba32>>, "Failed to tileize tiles.png.");

    // Process metatiles in groups of 12 entries
    for (std::size_t metatile_idx = 0; metatile_idx < num_metatiles; ++metatile_idx) {
        Metatile<Rgba32> metatile{};
        const std::size_t base_entry_idx = metatile_idx * metatile::entries_per_metatile_triple;
        const std::optional<metatile::Layer> synthesized_layer =
            synthesized_layers.empty() ? std::nullopt : synthesized_layers[metatile_idx];

        // Decompiles the 4 entries of one layer group, starting at first_entry_idx, into the metatile via set_tile
        auto decompile_layer = [&](const metatile::Layer layer, const std::size_t first_entry_idx, auto set_tile) {
            const Rgba32 transparent_color =
                transparent_color_for(mode, layer, synthesized_layer, extrinsic_transparency_);
            for (std::size_t i = 0; i < metatile::tiles_per_metatile_layer; ++i) {
                const auto &entry = entries[first_entry_idx + i];
                const auto &index_tile = tiles[entry.tile_index()];
                auto flipped_tile = index_tile.flip(entry.h_flip(), entry.v_flip());
                set_tile(
                    i, color_tile_from_index_tile(flipped_tile, palettes[entry.palette_index()], transparent_color));
            }
        };

        decompile_layer(metatile::Layer::bottom, base_entry_idx, [&](std::size_t i, PixelTile<Rgba32> tile) {
            metatile.set_bottom(i, std::move(tile));
        });
        decompile_layer(
            metatile::Layer::middle,
            base_entry_idx + metatile::tiles_per_metatile_layer,
            [&](std::size_t i, PixelTile<Rgba32> tile) { metatile.set_middle(i, std::move(tile)); });
        decompile_layer(
            metatile::Layer::top,
            base_entry_idx + 2 * metatile::tiles_per_metatile_layer,
            [&](std::size_t i, PixelTile<Rgba32> tile) { metatile.set_top(i, std::move(tile)); });

        decompiled.push_back(std::move(metatile));
    }

    return decompiled;
}

} // namespace porytiles
