#pragma once

#include <filesystem>
#include <string>
#include <unistd.h>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles/infra/services/editor_launcher.hpp"
#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/terminal_width.hpp"
#include "porytiles/xcut/di/components.hpp"
#include "porytiles/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

#include "command.hpp"
#include "option.hpp"

/// @brief Opens a tileset's YAML config file in the user's preferred editor.
///
/// @details
/// EditTilesetConfigCommand resolves the editor via the EditorLauncher service and opens the tileset's @c config.yaml
/// (or @c config.local.yaml with @c --local). When the tileset directory or the config file does not exist yet, the
/// command creates them first, so a config can be prepared for a tileset before it is created or imported.
///
/// Unlike the other tileset commands, this one skips the TilesetCommandEnv initialization. Since that process eagerly
/// validates every YAML config file, a currently-invalid config would fail the command before the editor could
/// open, which is is not a great user experience given that this command is for editing said configs (the user may be
/// using the command to fix an invalid config).
class EditTilesetConfigCommand final : public Command {
  public:
    explicit EditTilesetConfigCommand(CLI::App &parent_app)
        : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset whose config to edit")->required();
        cmd.add_flag("--local", local_, "Edit the tileset's 'config.local.yaml' instead of its shared 'config.yaml'.");
        project_root_opt_.RegisterOpt(cmd);
    }

  private:
    void Run() override
    {
        using namespace porytiles;

        fruit::Injector<TextFormatter> injector{di::get_formatter_component, !isatty(STDERR_FILENO)};
        auto *text_formatter = injector.get<TextFormatter *>();
        const StderrStyledUserDiagnostics diag{text_formatter, resolve_terminal_width(STDERR_FILENO)};

        const auto edit_result = edit_config(text_formatter, diag);
        if (!edit_result.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to edit config for tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                edit_result};
            diag.fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
    }

    [[nodiscard]] porytiles::ChainableResult<void>
    edit_config(const porytiles::TextFormatter *text_formatter, const porytiles::UserDiagnostics &diag) const
    {
        using namespace porytiles;

        // The name becomes a directory component below, so reject anything that could escape the tilesets directory.
        if (tileset_name_.find('/') != std::string::npos || tileset_name_.find('\\') != std::string::npos ||
            tileset_name_.find("..") != std::string::npos) {
            return FormattableError{"Invalid tileset name '{}'.", FormatParam{tileset_name_, Style::bold}};
        }

        const auto project_root = project_root_opt_.project_root();
        const auto tileset_dir = project_root / "porytiles" / "tilesets" / tileset_name_;
        const auto config_file = tileset_dir / (local_ ? "config.local.yaml" : "config.yaml");

        // Editing a config for a tileset that does not exist yet is supported (the config applies once the tileset
        // is created or imported), but warn so a misspelled name does not silently create a junk directory.
        const ProjectTilesetMetadataProvider metadata_provider{project_root, text_formatter, &diag};
        if (!metadata_provider.exists(tileset_name_)) {
            diag.warning(
                "edit-config-missing-tileset",
                "Tileset '{}' does not exist in this project. Creating its config anyway.",
                FormatParam{tileset_name_, Style::bold});
        }

        constexpr EditorLauncher launcher{};
        return launcher.create_and_edit_file(config_file);
    }

    static constexpr auto command_name = "edit-tileset-config";
    static constexpr auto command_desc = "Open a tileset's YAML config file in your editor.";
    static constexpr auto command_group = "UTILITIES";
    std::string tileset_name_;
    bool local_{false};
    OptProjectRoot project_root_opt_;
};
