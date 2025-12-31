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
#include "porytiles2/utilities/c_parser/incbin_declaration.hpp"
#include "porytiles2/utilities/c_parser/struct_initializer_declaration.hpp"
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
        : project_root_{std::move(project_root)}, format_{format}, diag_{diag}
    {
    }

    /*
     * Porymap artifacts
     */
    [[nodiscard]] ChainableResult<ArtifactKey> key_for_metatiles_bin(const std::string &name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_metatile_attributes_bin(const std::string &name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_tiles_png(const std::string &name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey>
    key_for_porymap_pal_n(const std::string &name, std::size_t index) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_porymap_anim_frame(
        const std::string &name, const std::string &anim_name, std::size_t frame_index) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_generated_anim_code(const std::string &name) const override;

    /*
     * Porytiles artifacts
     */
    [[nodiscard]] ChainableResult<ArtifactKey> key_for_bottom_png(const std::string &name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_middle_png(const std::string &name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_top_png(const std::string &name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_attributes_csv(const std::string &name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey>
    key_for_porytiles_pal_n(const std::string &name, std::size_t index) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_porytiles_anim_frame(
        const std::string &name, const std::string &anim_name, std::size_t frame_index) const override;

    [[nodiscard]] ChainableResult<ArtifactKey>
    key_for_porytiles_anim_key_frame(const std::string &name, const std::string &anim_name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_anim_yaml(const std::string &name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_config(const std::string &name) const override;

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_local_config(const std::string &name) const override;

    /*
     * Utilities
     */
    [[nodiscard]] bool artifact_exists(const ArtifactKey &key) const override;

    [[nodiscard]] bool tileset_exists(const std::string &name) const override;

    [[nodiscard]] ChainableResult<std::set<std::string>>
    discover_porytiles_anims(const std::string &name) const override;

    [[nodiscard]] ChainableResult<std::set<int>>
    discover_porytiles_anim_frames(const std::string &name, const std::string &anim_name) const override;

    [[nodiscard]] ChainableResult<std::set<std::string>> discover_porymap_anims(const std::string &name) const override;

    [[nodiscard]] ChainableResult<std::set<int>>
    discover_porymap_anim_frames(const std::string &name, const std::string &anim_name) const override;

    [[nodiscard]] ChainableResult<std::optional<AnimationCallbackInfo>>
    animation_callback_info_for(const std::string &name) const override;

    /**
     * @brief Returns the filesystem path to the root directory of a tileset.
     *
     * @details
     * This method provides the base directory path where all artifacts for a specific tileset are stored within the
     * project's filesystem structure. This is specific to the filesystem-based implementation, as other backing stores
     * may not have a concept of a single root directory for tileset artifacts.
     *
     * @param name The name of the tileset
     * @return The filesystem path to the tileset's root directory
     */
    [[nodiscard]] ChainableResult<std::filesystem::path> tileset_root(const std::string &name) const;

  private:
    [[nodiscard]] ChainableResult<TilesetArtifactPaths> artifact_paths(const std::string &name) const;

    std::filesystem::path project_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;

    // Lazy-loaded caches (mutable for const methods)
    mutable bool headers_parsed_{false};
    mutable bool incbins_parsed_{false};
    mutable std::map<std::string, StructInitializerDeclaration> tileset_structs_; // variable_name -> struct
    mutable std::map<std::string, IncbinDeclaration> incbin_vars_;                // variable_name -> incbin
};

} // namespace porytiles2
