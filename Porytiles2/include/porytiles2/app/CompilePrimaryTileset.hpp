#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/repos/PorymapTilesetRepo.hpp"
#include "porytiles2/domain/repos/PorytilesTilesetRepo.hpp"
#include "porytiles2/domain/services/TilesetCompilerService.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Use case for compiling a primary PorytilesTileset.
 */
class CompilePrimaryTileset {
  public:
    /**
     * @brief Constructs a CompilePrimaryTileset use case with the given repositories and compilation service.
     *
     * @param porytiles_repo A pointer to the PorytilesTilesetRepo for this use case.
     * @param porymap_repo A pointer to the PorymapTilesetRepo for this use case.
     * @param compiler_service A pointer to the TilesetCompilerService for this use case.
     */
    CompilePrimaryTileset(std::unique_ptr<PorytilesTilesetRepo> porytiles_repo,
                          std::unique_ptr<PorymapTilesetRepo> porymap_repo,
                          std::unique_ptr<TilesetCompilerService> compiler_service)
        : porytiles_repo_{std::move(porytiles_repo)}, porymap_repo_{std::move(porymap_repo)},
          compiler_service_{std::move(compiler_service)} {}

    /**
     * @brief Compiles the primary tileset with the given tileset name.
     *
     * @details
     * Given a primary tileset by name, compile the PorytilesTileset assets into PorymapTileset assets. Uses the use
     * case's configured repos to load and save the tileset assets. Uses the given TilesetCompilationService to perform
     * the compilation operation.
     *
     * @param tileset The name of the primary tileset to compile.
     * @return An empty Result on success, otherwise an error description.
     */
    [[nodiscard]] Result<void> Compile(const std::string &tileset) const;

  private:
    std::unique_ptr<PorytilesTilesetRepo> porytiles_repo_;
    std::unique_ptr<PorymapTilesetRepo> porymap_repo_;
    std::unique_ptr<TilesetCompilerService> compiler_service_;
};

} // namespace porytiles
