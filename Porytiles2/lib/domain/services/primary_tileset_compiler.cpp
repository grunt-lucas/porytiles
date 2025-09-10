#include "porytiles2/domain/services/primary_tileset_compiler.hpp"

#include <expected>
#include <memory>

#include "porytiles2/domain/model/porymap_tileset_component.hpp"
#include "porytiles2/domain/model/porytiles_tileset_component.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

Result<std::unique_ptr<PorymapTilesetComponent>>
PrimaryTilesetCompiler::compile(const PorytilesTilesetComponent &tileset)
{
    // TODO: implement for real

    // Push some dummy values into the component
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, false, false});
    porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, true, true});

    return porymap_component;
}

Result<std::unique_ptr<PorymapTilesetComponent>> PrimaryTilesetCompiler::compile_incremental(
    const PorytilesTilesetComponent &tileset, const PorymapTilesetComponent &context)
{
    return std::unexpected("TODO: implement");
}

} // namespace porytiles2
