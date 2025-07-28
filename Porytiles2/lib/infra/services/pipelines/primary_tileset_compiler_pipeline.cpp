#include "porytiles2/infra/services/pipelines/primary_tileset_compiler_pipeline.hpp"

#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

Result<std::unique_ptr<PorymapTilesetComponent>>
PrimaryTilesetCompilerPipeline::compile(const PorytilesTilesetComponent &tileset) {
    panic("TODO: unimplemented");
}

Result<std::unique_ptr<PorymapTilesetComponent>> compile_incremental(const PorytilesTilesetComponent &tileset,
                                                                     const PorymapTilesetComponent &context) {
    panic("TODO: unimplemented");
}

} // namespace porytiles2
