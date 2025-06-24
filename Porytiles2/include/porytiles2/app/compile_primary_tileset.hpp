#pragma once

#include <memory>
#include <string>

#include <porytiles2/domain/repos/porymap_tileset_repo.hpp>
#include <porytiles2/domain/repos/porytiles_tileset_repo.hpp>
#include <porytiles2/domain/services/tileset_compiler_service.hpp>

namespace porytiles {

class CompilePrimaryTileset {
  public:
    CompilePrimaryTileset(std::unique_ptr<PorytilesTilesetRepo> porytiles_repo,
                          std::unique_ptr<PorymapTilesetRepo> porymap_repo,
                          std::unique_ptr<TilesetCompilerService> compiler_service_)
        : porytiles_repo_{std::move(porytiles_repo)}, porymap_repo_{std::move(porymap_repo)},
          compiler_service_{std::move(compiler_service_)} {}

    void Compile(const std::string &tileset) const;

  private:
    std::unique_ptr<PorytilesTilesetRepo> porytiles_repo_;
    std::unique_ptr<PorymapTilesetRepo> porymap_repo_;
    std::unique_ptr<TilesetCompilerService> compiler_service_;
};

} // namespace porytiles
