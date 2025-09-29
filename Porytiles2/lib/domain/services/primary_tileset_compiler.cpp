#include "porytiles2/domain/services/primary_tileset_compiler.hpp"

#include <memory>
#include <vector>

#include "porytiles2/domain/model/normalized_tile.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/services/color_index_map_builder.hpp"
#include "porytiles2/domain/services/rgba_layer_image_metatileizer.hpp"
#include "porytiles2/domain/services/rgba_tile_normalizer.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetCompiler::compile(const Tileset &tileset)
{
    // Initialize all the services we need
    RgbaLayerImageMetatileizer metatileizer{};
    RgbaTileNormalizer normalizer{};
    ColorIndexMapBuilder color_index_map_builder{};

    // Transform the tileset layer images into a sequence of metatiles
    const auto metatiles_result = metatileizer.metatileize(
        tileset.porytiles_component().bottom(),
        tileset.porytiles_component().middle(),
        tileset.porytiles_component().top());
    if (!metatiles_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>::chain_together(
            FormattableError{"failed to metatileize input layer images"}, metatiles_result);
    }
    const auto &metatiles = metatiles_result.value();

    // Compute NormalizedTiles from the input metatiles
    std::vector<NormalizedTile<Rgba32>> norm_tiles{};
    for (const auto &metatile : metatiles) {
        for (const auto &bottom_tile : metatile.bottom()) {
            const auto &norm_result = normalizer.normalize(RgbaTile{bottom_tile}, rgba_magenta);
            if (!norm_result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>::chain_together(
                    FormattableError{"normalization failed: TODO print some tile info here"}, norm_result);
            }
            norm_tiles.push_back(norm_result.value());
        }
        for (const auto &middle_tile : metatile.middle()) {
            const auto &norm_result = normalizer.normalize(RgbaTile{middle_tile}, rgba_magenta);
            if (!norm_result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>::chain_together(
                    FormattableError{"normalization failed: TODO print some tile info here"}, norm_result);
            }
            norm_tiles.push_back(norm_result.value());
        }
        for (const auto &top_tile : metatile.top()) {
            const auto &norm_result = normalizer.normalize(RgbaTile{top_tile}, rgba_magenta);
            if (!norm_result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>::chain_together(
                    FormattableError{"normalization failed: TODO print some tile info here"}, norm_result);
            }
            norm_tiles.push_back(norm_result.value());
        }
    }

    const auto &color_index_map = color_index_map_builder.build_map(norm_tiles, rgba_magenta);

    // TODO: set up these components correctly, for now we just use some dummy values
    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>(tileset.porytiles_component());
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, false, false});
    porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, true, true});

    // TODO: The resulting PorymapTilesetComponent may be incomplete. E.g., the user may have specified PLA
    // files; they will be present on disk. We don't want to clobber them when saving the newly compiled
    // component. So we'll need to pull them from the original component and inject them into this one before
    // returning. One way around this would be to add PLA files to the Porytiles component. Compilation can simply copy
    // them over. We'll also have to handle this on the import side. That is, when importing a tileset that contains PLA
    // files, we need to make sure to copy them into the new Porytiles component.

    auto new_tileset =
        std::make_unique<Tileset>(tileset.name(), std::move(porytiles_component), std::move(porymap_component));

    return new_tileset;
}

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetCompiler::compile_incremental(const Tileset &tileset)
{
    // TODO: implement for real
    // Pipeline pipeline{};
    panic("TODO: implement");
}

} // namespace porytiles2
