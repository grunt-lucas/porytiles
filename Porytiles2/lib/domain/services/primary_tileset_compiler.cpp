#include "porytiles2/domain/services/primary_tileset_compiler.hpp"

#include <array>
#include <memory>
#include <ranges>
#include <vector>

#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/color_index_map_builder.hpp"
#include "porytiles2/domain/services/pack_set_generator.hpp"
#include "porytiles2/domain/services/rgba_layer_image_metatileizer.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetCompiler::compile(const Tileset &tileset)
{
    // Initialize all the compilation services
    RgbaLayerImageMetatileizer metatileizer{};
    ColorIndexMapBuilder color_index_map_builder{};

    // Transform the tileset layer images into a sequence of metatiles
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatileizer.metatileize(
            tileset.porytiles_component().bottom(),
            tileset.porytiles_component().middle(),
            tileset.porytiles_component().top()),
        "failed to metatileize input layer images",
        std::unique_ptr<Tileset>);

    // Decompose metatiles into a raw tiles vector
    std::vector<RgbaTile> tiles{};
    tiles.reserve(metatiles.size() * metatile::tiles_per_metatile);
    for (const auto &metatile : metatiles) {
        const auto decomposed = metatile.decompose();
    }

    // Compute NormalizedTiles from the input metatiles
    // PT_TRY_ASSIGN_CHAIN_ERR(
    //     norm_tiles,
    //     normalizer.batch_normalize(metatiles, rgba_magenta),
    //     "metatile normalization failed",
    //     std::unique_ptr<Tileset>);

    // Create PackSets for the bin packing step
    // const auto &color_index_map = color_index_map_builder.build_map(norm_tiles, rgba_magenta);
    // ColorSetBuilder color_set_builder{text_formatter_};
    // PackSetGenerator assignable_tile_generator{&color_set_builder};
    // std::vector<PackSet> assignable_tiles = assignable_tile_generator.generate(norm_tiles, color_index_map);

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
