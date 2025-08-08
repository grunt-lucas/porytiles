#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"

namespace porytiles2 {

class ProjectTilesetKeyProvider : public TilesetArtifactKeyProvider {
  public:
    explicit ProjectTilesetKeyProvider(std::filesystem::path project_root) : project_root_{std::move(project_root)} {}

    /**
     * @brief Computes the path to the given primary tileset's directory.
     *
     * @param tileset The name of the primary tileset.
     * @return The path to the given primary tileset's directory.
     */
    [[nodiscard]] std::filesystem::path primary_tileset_directory(const std::string &tileset) const;

    /**
     * @brief Computes the path to the given primary tileset's Porytiles `bottom.png` file.
     *
     * @param tileset The name of the primary tileset.
     * @return The path to the given primary tileset's `bottom.png` file.
     */
    [[nodiscard]] std::filesystem::path primary_bottom_png(const std::string &tileset) const;

    /**
     * @brief Computes the path to the given primary tileset's Porytiles `middle.png` file.
     *
     * @param tileset The name of the primary tileset.
     * @return The path to the given primary tileset's `middle.png` file.
     */
    [[nodiscard]] std::filesystem::path primary_middle_png(const std::string &tileset) const;

    /**
     * @brief Computes the path to the given primary tileset's Porytiles `top.png` file.
     *
     * @param tileset The name of the primary tileset.
     * @return The path to the given primary tileset's `top.png` file.
     */
    [[nodiscard]] std::filesystem::path primary_top_png(const std::string &tileset) const;

    /**
     * @brief Computes the path to the given secondary tileset's directory.
     *
     * @param tileset The name of the secondary tileset.
     * @return The path to the given secondary tileset's directory.
     */
    [[nodiscard]] std::filesystem::path secondary_tileset_directory(const std::string &tileset) const;

    /**
     * @brief Computes the path to the given secondary tileset's Porytiles `bottom.png` file.
     *
     * @param tileset The name of the secondary tileset.
     * @return The path to the given secondary tileset's `bottom.png` file.
     */
    [[nodiscard]] std::filesystem::path secondary_bottom_png(const std::string &tileset) const;

    /**
     * @brief Computes the path to the given secondary tileset's Porytiles `middle.png` file.
     *
     * @param tileset The name of the secondary tileset.
     * @return The path to the given secondary tileset's `middle.png` file.
     */
    [[nodiscard]] std::filesystem::path secondary_middle_png(const std::string &tileset) const;

    /**
     * @brief Computes the path to the given secondary tileset's Porytiles `top.png` file.
     *
     * @param tileset The name of the secondary tileset.
     * @return The path to the given secondary tileset's `top.png` file.
     */
    [[nodiscard]] std::filesystem::path secondary_top_png(const std::string &tileset) const;

    /**
     * @brief Computes the path to the project `metatile_behaviors.h` file.
     *
     * @details
     * While ProjectPaths::BehaviorsHeader by default assumes that `metatile_behaviors.h` lives in
     * the standard location (i.e. `include/constants`), it can optionally take into account a
     * user-supplied override path or file via ProjectPaths::SetBehaviorsHeaderOverridePath and
     * ProjectPaths::SetBehaviorsHeaderOverrideFile.
     *
     * @return The path to the project's `metatile_behaviors.h` file.
     */
    [[nodiscard]] std::filesystem::path behaviors_header() const;

    /**
     * @brief Sets an override path for the metatile behaviors header.
     *
     * @details
     * The supplied override path should point to the directory that contains the
     * `metatile_behaviors.h` file, and it must be relative to the project root. If you need to
     * override the file name itself, see ProjectPaths::SetBehaviorsHeaderOverrideFile.
     *
     * @param override The override behaviors header path.
     */
    void set_behaviors_header_override_path(std::filesystem::path override);

    /**
     * @brief Sets an override file name for the metatile behaviors header.
     *
     * @param override The override behaviors header file name.
     */
    void set_behaviors_header_override_file(std::string override);

    /**
     * @brief Computes the path to the project `graphics.h` file.
     *
     * @details
     * This file contains tileset graphics declarations including palette and tile data.
     * Located at `src/data/tilesets/graphics.h` in the pokeemerald project structure.
     *
     * @return The path to the project's `graphics.h` file.
     */
    [[nodiscard]] std::filesystem::path graphics_header() const;

    /**
     * @brief Computes the path to the project `headers.h` file.
     *
     * @details
     * This file contains tileset struct definitions and tileset configuration data.
     * Located at `src/data/tilesets/headers.h` in the pokeemerald project structure.
     *
     * @return The path to the project's `headers.h` file.
     */
    [[nodiscard]] std::filesystem::path headers_header() const;

    /**
     * @brief Computes the path to the project `metatiles.h` file.
     *
     * @details
     * This file contains metatile and metatile attribute array declarations.
     * Located at `src/data/tilesets/metatiles.h` in the pokeemerald project structure.
     *
     * @return The path to the project's `metatiles.h` file.
     */
    [[nodiscard]] std::filesystem::path metatiles_header() const;

  private:
    std::filesystem::path project_root_;
    std::optional<std::filesystem::path> behaviors_header_override_path_;
    std::optional<std::string> behaviors_header_override_file_;
};

} // namespace porytiles2
