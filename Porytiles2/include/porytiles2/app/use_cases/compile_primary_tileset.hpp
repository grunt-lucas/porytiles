#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Use case for compiling a primary Tileset.
 */
class CompilePrimaryTileset {
  public:
    /**
     * @brief Constructs a CompilePrimaryTileset use case with the given repositories and services.
     *
     * @param tileset_repo A pointer to the TilesetRepo for this use case.
     * @param compiler A pointer to the PrimaryTilesetCompiler for this use case.
     */
    CompilePrimaryTileset(std::unique_ptr<TilesetRepo> tileset_repo, std::unique_ptr<PrimaryTilesetCompiler> compiler)
        : tileset_repo_{std::move(tileset_repo)}, compiler_{std::move(compiler)} {}

    /**
     * @brief Compiles the primary Tileset with the given tileset name.
     *
     * @details
     * Given a primary tileset by name, compile the PorytilesTileset assets into PorymapTileset assets. Uses the use
     * case's configured repos to load and save the tileset assets. Uses the given TilesetCompilationService to perform
     * the compilation operation.
     *
     * @param tileset_name The name of the primary Tileset to compile.
     * @return An empty Result on success, otherwise an error description.
     */
    [[nodiscard]] Result<void> compile(const std::string &tileset_name) const;

  private:
    std::unique_ptr<TilesetRepo> tileset_repo_;
    std::unique_ptr<PrimaryTilesetCompiler> compiler_;
};

} // namespace porytiles2
