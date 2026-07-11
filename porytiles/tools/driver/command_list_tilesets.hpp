#pragma once

#include <filesystem>
#include <iostream>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/null_user_diagnostics.hpp"

#include "command.hpp"

/// @brief Lists tileset names in the project.
///
/// @details
/// ListTilesetsCommand provides a way to list all tilesets in a project. This is useful
/// for scripting, getting an overview of the project, and for shell completion scripts.
///
/// The command supports filtering by management status:
/// - `all`: Return all tilesets found in the project (default)
/// - `managed`: Return only tilesets that have been imported/created by Porytiles
/// - `unmanaged`: Return only tilesets not yet managed by Porytiles
///
/// CRITICAL: This command must NEVER write to stderr, as that would corrupt shell
/// completion results. All errors are silently ignored.
class ListTilesetsCommand final : public Command {
  public:
    explicit ListTilesetsCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();

        cmd.add_option("-C,--project-root", project_root_, "Project root directory")
            ->default_val(".")
            ->capture_default_str();

        cmd.add_option("--filter", filter_, "Filter mode: all, managed, or unmanaged")
            ->default_val("all")
            ->check(CLI::IsMember({"all", "managed", "unmanaged"}))
            ->capture_default_str();

        cmd.add_option("--prefix", prefix_, "Only show tilesets starting with this prefix")->default_val("");
    }

    void Run() override
    {
        using namespace porytiles;

        // Validate project root exists - check early before creating objects
        const std::filesystem::path project_path{project_root_};
        if (!exists(project_path) || !is_directory(project_path)) {
            // Silent exit - no output on errors
            return;
        }

        // Use PlainTextFormatter (no colors needed for completion output)
        PlainTextFormatter formatter{};
        NullUserDiagnostics diag{&formatter};

        // Get all tileset names from the project
        ProjectTilesetMetadataProvider provider{project_path, &formatter, &diag};
        auto result = provider.tilesets();

        if (!result.has_value()) {
            // Silent exit - no output on errors
            return;
        }

        const auto &tileset_names = result.value();

        // Output matching tileset names
        for (const auto &name : tileset_names) {
            // Apply prefix filter
            if (!prefix_.empty() && !name.starts_with(prefix_)) {
                continue;
            }

            // Apply management status filter
            if (filter_ == "managed") {
                if (!is_managed_tileset(project_path, name)) {
                    continue;
                }
            }
            else if (filter_ == "unmanaged") {
                if (is_managed_tileset(project_path, name)) {
                    continue;
                }
            }
            // filter_ == "all" means no filtering

            std::cout << name << "\n";
        }
    }

  private:
    static constexpr auto kCommandName = "list-tilesets";
    static constexpr auto kCommandDesc = "List tilesets in the project.";
    static constexpr auto kCommandGroup = "UTILITIES";

    std::string project_root_;
    std::string filter_;
    std::string prefix_;

    /// @brief Checks if a tileset is managed by Porytiles.
    ///
    /// @details
    /// A tileset is considered "managed" if it has a tileset-manifest.json file in its
    /// porytiles directory. This file is created when a tileset is imported or created
    /// via Porytiles commands.
    ///
    /// The porytiles directory structure uses the full tileset name (e.g., gTileset_General)
    /// as the subdirectory name.
    ///
    /// @param project_root The project root path.
    /// @param tileset_name The name of the tileset (e.g., "gTileset_General"). Must be a valid
    ///        tileset name from ProjectTilesetMetadataProvider (no path separators or traversal sequences).
    /// @return True if the tileset is managed by Porytiles.
    [[nodiscard]] static bool
    is_managed_tileset(const std::filesystem::path &project_root, const std::string &tileset_name)
    {
        // Security: Validate tileset_name doesn't contain path traversal sequences.
        // Tileset names from ProjectTilesetMetadataProvider should be safe, but validate anyway.
        if (tileset_name.empty() || tileset_name.find('/') != std::string::npos ||
            tileset_name.find('\\') != std::string::npos || tileset_name.find("..") != std::string::npos) {
            return false;
        }

        // Check for porytiles manifest file using full tileset name as directory
        const auto manifest_path = project_root / "porytiles" / "tilesets" / tileset_name / "tileset-manifest.json";

        return exists(manifest_path);
    }
};
