#pragma once

#include <memory>

#include <porytiles2/domain/aggregates/porymap_tileset.hpp>
#include <porytiles2/domain/aggregates/porytiles_tileset.hpp>
#include <porytiles2/templates/result.hpp>

namespace porytiles {

class TilesetCompilerService {
  public:
    virtual ~TilesetCompilerService() = default;

    virtual Result<std::unique_ptr<PorymapTileset>> CompilePrimary(const PorytilesTileset &porytilesTileset) = 0;
};

} // namespace porytiles
