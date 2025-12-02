#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include "gsl/pointers"

#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"
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
 * Class precondition: the tileset_name parameter in each method below (except tileset_exists) must refer to an existing
 * tileset on-disk. If no tileset corresponds to the given tileset_name, ProjectTilesetKeyProvider will panic.
 */
class ProjectTilesetArtifactKeyProvider final : public TilesetArtifactKeyProvider {
  public:
    explicit ProjectTilesetArtifactKeyProvider(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : project_root_{std::move(project_root)}, format_{format}, diag_{diag}
    {
    }

    [[nodiscard]] ArtifactKey key_for_metatiles_bin(const std::string &tileset_name) const override;

    [[nodiscard]] ArtifactKey key_for_metatile_attributes_bin(const std::string &tileset_name) const override;

    [[nodiscard]] ArtifactKey key_for_tiles_png(const std::string &tileset_name) const override;

    [[nodiscard]] ArtifactKey key_for_porymap_pal_n(const std::string &tileset_name, unsigned int index) const override;

    [[nodiscard]] ArtifactKey key_for_porymap_anim_frame(
        const std::string &tileset_name, const std::string &anim_name, int frame_index) const override;

    [[nodiscard]] ArtifactKey key_for_bottom_png(const std::string &tileset_name) const override;

    [[nodiscard]] ArtifactKey key_for_middle_png(const std::string &tileset_name) const override;

    [[nodiscard]] ArtifactKey key_for_top_png(const std::string &tileset_name) const override;

    [[nodiscard]] ArtifactKey key_for_attributes_csv(const std::string &tileset_name) const override;

    [[nodiscard]] ArtifactKey
    key_for_porytiles_pal_n(const std::string &tileset_name, unsigned int index) const override;

    [[nodiscard]] ArtifactKey key_for_porytiles_anim_frame(
        const std::string &tileset_name, const std::string &anim_name, int frame_index) const override;

    [[nodiscard]] ArtifactKey key_for_config(const std::string &tileset_name) const override;

    [[nodiscard]] ArtifactKey key_for_local_config(const std::string &tileset_name) const override;

    [[nodiscard]] bool artifact_exists(const ArtifactKey &key) const override;

    [[nodiscard]] bool tileset_exists(const std::string &tileset_name) const override;

    [[nodiscard]] std::set<std::string> discover_porytiles_anims(const std::string &tileset_name) const override;

    [[nodiscard]] std::set<int>
    discover_porytiles_anim_frames(const std::string &tileset_name, const std::string &anim_name) const override;

    [[nodiscard]] std::set<std::string> discover_porymap_anims(const std::string &tileset_name) const override;

    [[nodiscard]] std::set<int>
    discover_porymap_anim_frames(const std::string &tileset_name, const std::string &anim_name) const override;

    /**
     * @brief Returns the filesystem path to the root directory of a tileset.
     *
     * @details
     * This method provides the base directory path where all artifacts for a specific tileset are stored within the
     * project's filesystem structure. This is specific to the filesystem-based implementation, as other backing stores
     * may not have a concept of a single root directory for tileset artifacts.
     *
     * @param tileset_name The name of the tileset
     * @return The filesystem path to the tileset's root directory
     */
    [[nodiscard]] std::filesystem::path tileset_root(const std::string &tileset_name) const;

  private:
    std::filesystem::path project_root_;

    // TODO: implement configurable override paths for users who have changed the pokeemerald project structure
    // std::optional<std::filesystem::path> behaviors_header_override_path_;
    // std::optional<std::string> behaviors_header_override_file_;

    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles2
