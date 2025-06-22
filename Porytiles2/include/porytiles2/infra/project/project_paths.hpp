#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>

namespace porytiles {

/**
 * @brief Provides `pokeemerald` project path computational functionality based on a given project root.
 */
class ProjectPaths {
  public:
    explicit ProjectPaths(std::filesystem::path project_root) : project_root_{std::move(project_root)} {}

    /**
     * @brief Computes the path to the project `metatile_behaviors.h` file.
     *
     * @details
     * While BehaviorsHeader by default assumes that `metatile_behaviors.h` lives in the standard location (i.e.
     * `include/constants`), it can optionally take into account a user-supplied override path or file via
     * SetBehaviorsHeaderOverridePath and SetBehaviorsHeaderOverrideFile.
     *
     * @return The path to the project's `metatile_behaviors.h` file.
     */
    [[nodiscard]] std::filesystem::path BehaviorsHeader() const;

    /**
     * @brief Sets an override path for the metatile behaviors header.
     *
     * @details
     * The supplied override path should point to the directory that contains the `metatile_behaviors.h` file, and it
     * must be relative to the project root. If you need to override the file name itself, see
     * SetBehaviorsHeaderOverrideFile.
     *
     * @param override The override behaviors header path.
     */
    void SetBehaviorsHeaderOverridePath(std::filesystem::path override);

    /**
     * @brief Sets an override file name for the metatile behaviors header.
     *
     * @param override The override behaviors header file name.
     */
    void SetBehaviorsHeaderOverrideFile(std::string override);

  private:
    std::filesystem::path project_root_;
    std::optional<std::filesystem::path> behaviors_header_override_path_;
    std::optional<std::string> behaviors_header_override_file_;
};

} // namespace porytiles
