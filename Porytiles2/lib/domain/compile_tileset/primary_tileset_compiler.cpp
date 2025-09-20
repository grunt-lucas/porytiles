#include "porytiles2/domain/compile_tileset/primary_tileset_compiler.hpp"

#include <expected>
#include <memory>
#include <vector>

#include "porytiles2/domain/compile_tileset/operations/construct_rgba_metatiles_op.hpp"
#include "porytiles2/domain/compile_tileset/operations/tileset_supplier_op.hpp"
#include "porytiles2/domain/model/porymap_tileset_component.hpp"
#include "porytiles2/domain/model/porytiles_tileset_component.hpp"
#include "porytiles2/domain/orchestration/pipeline.hpp"
#include "porytiles2/templates/error.hpp"
#include "porytiles2/templates/panic.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<PorymapTilesetComponent>>
PrimaryTilesetCompiler::compile(const PorytilesTilesetComponent &tileset)
{
    std::vector<Operation *> ops;

    // Supplies three layer images from the given tileset
    TilesetSupplierOp tileset_supplier_op{&tileset};
    ops.push_back(&tileset_supplier_op);

    // Take the layer images and construct a vector of rgba metatiles
    ConstructRgbaMetatilesOp construct_meta_op{};
    ops.push_back(&construct_meta_op);

    // Run the pipeline
    const Pipeline pipeline{ops};
    auto pipeline_result = pipeline.run();
    if (!pipeline_result.has_value()) {
        return ChainableResult<std::unique_ptr<PorymapTilesetComponent>>::chain_together(
            BasicError{"Failed to compile primary tileset"}, pipeline_result);
    }

    // Push some dummy values into the component
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, false, false});
    porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, true, true});

    return porymap_component;
}

ChainableResult<std::unique_ptr<PorymapTilesetComponent>> PrimaryTilesetCompiler::compile_incremental(
    const PorytilesTilesetComponent &tileset, const PorymapTilesetComponent &context)
{
    // TODO: implement for real
    // Pipeline pipeline{};
    panic("TODO: implement");
}

} // namespace porytiles2
