#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/config/config.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Use case for importing a primary Tileset.
 */
class ImportPrimaryTileset {
  public:
    /**
     * @brief Constructs an ImportPrimaryTileset use case with the given repositories and services.
     *
     * @param tileset_repo A pointer to the TilesetRepo for this use case
     * @param compiler A pointer to the PrimaryTilesetCompiler for this use case
     * @param config A pointer to the Config for this use case
     */
    ImportPrimaryTileset(
        std::unique_ptr<TilesetRepo> tileset_repo,
        std::unique_ptr<PrimaryTilesetCompiler> compiler,
        std::unique_ptr<Config> config)
        : tileset_repo_{std::move(tileset_repo)}, compiler_{std::move(compiler)}, config_{std::move(config)} {}

    /**
     * @brief Imports the primary Tileset with the given tileset name.
     *
     * @param tileset_name The name of the primary Tileset to import
     * @return An empty Result on success, otherwise an error description
     */
    [[nodiscard]] Result<void> import(const std::string &tileset_name) const;

  private:
    std::unique_ptr<TilesetRepo> tileset_repo_;
    std::unique_ptr<PrimaryTilesetCompiler> compiler_;
    std::unique_ptr<Config> config_;
};

} // namespace porytiles2
