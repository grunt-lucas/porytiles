#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/asset_generator.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Use case for creating a primary Tileset.
 */
class CreatePrimaryTileset {
  public:
    /**
     * @brief Constructs a CreatePrimaryTileset use case with the given repositories and services.
     *
     * @param tileset_repo A pointer to the TilesetRepo for this use case.
     * @param compiler A pointer to the PrimaryTilesetCompiler for this use case.
     * @param asset_generator A pointer to the AssetGenerator for this use case.
     */
    CreatePrimaryTileset(
        std::unique_ptr<TilesetRepo> tileset_repo,
        std::unique_ptr<PrimaryTilesetCompiler> compiler,
        std::unique_ptr<AssetGenerator> asset_generator)
        : tileset_repo_{std::move(tileset_repo)}, compiler_{std::move(compiler)},
          asset_generator_{std::move(asset_generator)}
    {
    }

    /**
     * @brief Creates the primary Tileset with the given tileset name.
     *
     * @param tileset_name The name of the primary Tileset to create.
     * @return An empty Result on success, otherwise an error description.
     */
    [[nodiscard]] ChainableResult<void> create(const std::string &tileset_name) const;

  private:
    std::unique_ptr<TilesetRepo> tileset_repo_;
    std::unique_ptr<PrimaryTilesetCompiler> compiler_;
    std::unique_ptr<AssetGenerator> asset_generator_;
};

} // namespace porytiles2
