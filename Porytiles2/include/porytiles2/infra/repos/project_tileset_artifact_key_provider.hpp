#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>

#include "gsl/pointers"

#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"
#include "porytiles2/infra/repos/tileset_artifact_paths.hpp"
#include "porytiles2/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles2/utilities/c_parser/incbin_declaration.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Provides a pokeemerald project filesystem-based implementation for TilesetArtifactKeyProvider.
 *
 * @details
 * This class implements the TilesetArtifactKeyProvider interface to provide filesystem paths as keys for various
 * tileset artifacts. It operates within the context of a Pokémon Gen III decompilation project, discovering and
 * managing paths for animations, tiles, and other tileset components based on the project's directory structure.
 *
 * Tileset metadata and artifact paths are discovered dynamically by parsing C source files (headers.h, graphics.h,
 * metatiles.h) rather than relying on hardcoded filesystem path conventions.
 *
 * For Porymap artifacts (tiles, palettes, metatiles), paths are resolved from INCBIN declarations.
 * For Porytiles artifacts (bottom.png, middle.png, etc.), paths use a hardcoded relative location within each tileset.
 *
 * Class precondition: the tileset_name parameter in each method below (except tileset_exists) must refer to an existing
 * tileset on-disk. If no tileset corresponds to the given tileset_name, ProjectTilesetKeyProvider will panic.
 */
class ProjectTilesetArtifactKeyProvider final : public TilesetArtifactKeyProvider {
  public:
    /**
     * @brief Constructs a ProjectTilesetArtifactKeyProvider.
     *
     * @param project_root The root directory of the pokeemerald-style project
     * @param format Text formatter for styled output (non-owning, must outlive this)
     * @param diag Diagnostics handler for warnings (non-owning, must outlive this)
     */
    explicit ProjectTilesetArtifactKeyProvider(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : project_root_{std::move(project_root)}, format_{format}, diag_{diag},
          metadata_provider_{project_root_, format, diag}
    {
    }

    /*
     * Porymap artifacts
     */
    [[nodiscard]] ChainableResult<ArtifactKey> key_for_metatiles_bin(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey>
    key_for_metatile_attributes_bin(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_tiles_png(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey>
    key_for_porymap_pal_n(const std::string &tileset_name, std::size_t index) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_porymap_anim_frame(
        const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) const override;

    /**
     * @brief Returns the key for Porymap animation parameters.
     *
     * @details
     * For Porymap animations, generated_anim_code.h is the source of truth for animation
     * parameters. For first-time imports where this file doesn't exist, the reader will
     * fall back to tileset_anims.c. This method is an alias for key_for_generated_anim_code()
     * but with a more explicit name for the animation loading context.
     *
     * @param tileset_name The name of the tileset
     * @return Key for the generated_anim_code.h file
     */
    [[nodiscard]] ChainableResult<ArtifactKey>
    key_for_porymap_anim_params(const std::string &tileset_name) const override;

    /*
     * Porytiles artifacts
     */
    [[nodiscard]] ChainableResult<ArtifactKey> key_for_bottom_png(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_middle_png(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_top_png(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_attributes_csv(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey>
    key_for_porytiles_pal_n(const std::string &tileset_name, std::size_t index) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_porytiles_anim_frame(
        const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) const override;

    /**
     * @brief Returns the key for the anim.yaml file (Porytiles animation configuration).
     *
     * @details
     * The anim.yaml file stores animation parameters for the Porytiles component. It defines frame sequences, timing
     * parameters, and other configuration for each animation in the tileset. For Porytiles animations, the anim
     * parameters store is the source of truth for animation names, frame sequences, timing, and other parameters.
     *
     * @param tileset_name The name of the tileset
     * @return Key for the anim.yaml file
     */
    [[nodiscard]] ChainableResult<ArtifactKey>
    key_for_porytiles_anim_params(const std::string &tileset_name) const override;

    /*
     * Utilities
     */
    [[nodiscard]] bool artifact_exists(const ArtifactKey &key) const override;

    [[nodiscard]] ChainableResult<std::set<std::string>>
    discover_porymap_anims(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<std::set<std::string>>
    discover_porymap_anim_frames(const std::string &tileset_name, const std::string &anim_name) const override;

    [[nodiscard]] ChainableResult<std::set<std::string>>
    discover_porytiles_anims(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<std::set<std::string>>
    discover_porytiles_anim_frames(const std::string &tileset_name, const std::string &anim_name) const override;

    /*
     * Project Implementation-Specific Methods
     */

    /**
     * @brief Returns the filesystem path to the root directory of a tileset.
     *
     * @details
     * This method provides the base directory path where all artifacts for a specific tileset are stored within the
     * project's filesystem structure. Delegates to ProjectTilesetMetadataProvider for the actual path resolution.
     *
     * @param tileset_name The name of the tileset
     * @return The filesystem path to the tileset's root directory
     */
    [[nodiscard]] ChainableResult<std::filesystem::path> tileset_root(const std::string &tileset_name) const;

    /**
     * @brief Returns resolved filesystem paths for all Porymap artifacts of a tileset.
     *
     * @details
     * Delegates to ProjectTilesetMetadataProvider::artifact_paths_for() for the actual path resolution.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @pre tileset_name must refer to an existing tileset on disk
     * @return TilesetArtifactPaths containing resolved paths for all Porymap artifacts
     */
    [[nodiscard]] ChainableResult<TilesetArtifactPaths> artifact_paths_for(const std::string &tileset_name) const;

    /**
     * @brief Returns Porymap animation frame paths discovered from INCBIN declarations.
     *
     * @details
     * Discovers Porymap animation frames by parsing INCBIN declarations from the appropriate C source file:
     * - For Porytiles-managed tilesets: parses `<tileset_root>/include/generated_anim_code.h`
     * - For vanilla tilesets: parses `src/tileset_anims.c`
     *
     * The Porytiles-managed status is determined from tileset metadata. If the callback includes "PorytilesManaged_"
     * in its prefix, the tileset is considered Porytiles-managed.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @pre tileset_name must refer to an existing tileset on disk
     * @return AnimationFramePaths mapping animation names to ordered frame paths, or empty map if no animations
     */
    [[nodiscard]] ChainableResult<AnimationFramePaths>
    porymap_animation_frame_paths_for(const std::string &tileset_name) const;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;

    // Metadata provider for tileset struct parsing
    ProjectTilesetMetadataProvider metadata_provider_;

    // Lazy-loaded INCBIN cache (mutable for const methods)
    mutable bool incbins_parsed_{false};
    mutable std::map<std::string, IncbinDeclaration> incbin_vars_;
};

} // namespace porytiles2
