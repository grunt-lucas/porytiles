#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/asset_generator.hpp"
#include "porytiles2/domain/services/c_source_file_modifier.hpp"
#include "porytiles2/domain/services/c_source_generator.hpp"
#include "porytiles2/domain/services/header_file_parser.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/templates/result.hpp"

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
     * @param metadata_provider A pointer to the ArtifactMetadataService for this use case.
     * @param header_parser A pointer to the HeaderFileParser for this use case.
     * @param source_generator A pointer to the CSourceGenerator for this use case.
     * @param file_modifier A pointer to the CSourceFileModifier for this use case.
     */
    CreatePrimaryTileset(std::unique_ptr<TilesetRepo> tileset_repo, std::unique_ptr<PrimaryTilesetCompiler> compiler,
                         std::unique_ptr<AssetGenerator> asset_generator,
                         std::unique_ptr<ArtifactMetadataProvider> metadata_provider,
                         std::unique_ptr<HeaderFileParser> header_parser,
                         std::unique_ptr<CSourceGenerator> source_generator,
                         std::unique_ptr<CSourceFileModifier> file_modifier)
        : tileset_repo_{std::move(tileset_repo)}, compiler_{std::move(compiler)},
          asset_generator_{std::move(asset_generator)}, metadata_provider_{std::move(metadata_provider)},
          header_parser_{std::move(header_parser)}, source_generator_{std::move(source_generator)},
          file_modifier_{std::move(file_modifier)} {}

    /**
     * @brief Creates the primary Tileset with the given tileset name.
     *
     * @param tileset_name The name of the primary Tileset to create.
     * @return An empty Result on success, otherwise an error description.
     */
    [[nodiscard]] Result<void> create(const std::string &tileset_name) const;

  private:
    std::unique_ptr<TilesetRepo> tileset_repo_;
    std::unique_ptr<PrimaryTilesetCompiler> compiler_;
    std::unique_ptr<AssetGenerator> asset_generator_;
    std::unique_ptr<ArtifactMetadataProvider> metadata_provider_;
    std::unique_ptr<HeaderFileParser> header_parser_;
    std::unique_ptr<CSourceGenerator> source_generator_;
    std::unique_ptr<CSourceFileModifier> file_modifier_;
};

} // namespace porytiles2
