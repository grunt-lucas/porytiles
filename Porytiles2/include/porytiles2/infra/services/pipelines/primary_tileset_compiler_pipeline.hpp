#pragma once

#include "porytiles2/domain/services/primary_tileset_compiler.hpp"

namespace porytiles2 {

/**
 * @brief An implementation of PrimaryTilesetCompiler that uses the Pipeline orchestration class to perform the
 * compilation.
 */
class PrimaryTilesetCompilerPipeline final : PrimaryTilesetCompiler {
  public:
    Result<std::unique_ptr<PorymapTilesetComponent>> compile(const PorytilesTilesetComponent &tileset) override;

    Result<std::unique_ptr<PorymapTilesetComponent>>
    compile_incremental(const PorytilesTilesetComponent &tileset, const PorymapTilesetComponent &context) override;
};

} // namespace porytiles2
