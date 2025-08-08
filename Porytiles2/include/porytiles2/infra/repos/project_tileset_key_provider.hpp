#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"

#include <unordered_map>

namespace porytiles2 {

/**
 * @brief Provides a filesystem-based implementation for TilesetArtifactKeyProvider.
 *
 * @details
 * This class implements the TilesetArtifactKeyProvider interface to provide filesystem paths as keys for various
 * tileset artifacts. It operates within the context of a Pokémon Gen III decompilation project, discovering and
 * managing paths for animations, tiles, and other tileset components based on the project's directory structure.
 *
 * Class precondition: the tileset_name parameter in each method below must refer to an existing tileset on-disk. If no
 * tileset corresponds to the given tileset_name, ProjectTilesetKeyProvider will panic.
 */
class ProjectTilesetKeyProvider final : public TilesetArtifactKeyProvider {
  public:
    explicit ProjectTilesetKeyProvider(std::filesystem::path project_root) : project_root_{std::move(project_root)} {}

    [[nodiscard]] std::any key_for(const std::string &tileset_name, const TilesetArtifact &artifact) const override;

    [[nodiscard]] std::set<std::string> discover_porytiles_anims(const std::string &tileset_name) const override;

    [[nodiscard]] std::set<int> discover_porytiles_anim_frames(const std::string &tileset_name,
                                                               const std::string &anim_name) const override;

    [[nodiscard]] std::set<std::string> discover_porymap_anims(const std::string &tileset_name) const override;

    [[nodiscard]] std::set<int> discover_porymap_anim_frames(const std::string &tileset_name,
                                                             const std::string &anim_name) const override;

    [[nodiscard]] bool exists(const std::any &key) const override;

  private:
    std::filesystem::path project_root_;
    std::unordered_map<std::string, std::filesystem::path> subfolder_cache_;

    // TODO: implement configurable override paths for users who have changed the pokeemerald project structure
    // std::optional<std::filesystem::path> behaviors_header_override_path_;
    // std::optional<std::string> behaviors_header_override_file_;
};

} // namespace porytiles2
