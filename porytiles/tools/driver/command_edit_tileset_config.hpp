#pragma once

#include <filesystem>
#include <string>
#include <unistd.h>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles/domain/services/tileset_name_resolver.hpp"
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
/// command creates them first.
///
/// The tileset name argument accepts fuzzy forms and resolves to the canonical name declared in the project, which is
/// also the config directory name. The tileset must therefore exist, since the command creates files on disk and a
/// misspelled name would otherwise leave a junk tileset directory behind. Passing @c --allow-missing-tileset skips
/// resolution and uses the argument verbatim, so a config can be prepared for a tileset before it is created or
/// imported.
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
        cmd.add_option(
               "<tileset-name>", tileset_name_, "Name of the tileset whose config to edit (fuzzy names accepted)")
            ->required();
        cmd.add_flag("--local", local_, "Edit the tileset's 'config.local.yaml' instead of its shared 'config.yaml'.");
        cmd.add_flag(
            "--allow-missing-tileset",
            allow_missing_tileset_,
            "Edit config even when the tileset does not exist, to prepare a config before the tileset is created or "
            "imported.");
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

        // The fuzzy name argument resolves to the canonical name declared in the project, which is also the tileset
        // config directory name. Resolution replaces the old existence check: the command creates the tileset
        // directory and config file on disk, so a misspelled name would otherwise leave a junk directory behind.
        // --allow-missing-tileset skips resolution (there is no declared name to resolve against) and uses the
        // argument verbatim, for preparing the config of a not-yet-created tileset.
        std::string tileset_name = tileset_name_;
        if (!allow_missing_tileset_) {
            const ProjectTilesetMetadataProvider metadata_provider{project_root, text_formatter, &diag};
            PT_TRY_ASSIGN_PASS_ERR(tileset_names, metadata_provider.tilesets(), void);
            PT_TRY_ASSIGN_CHAIN_ERR(
                resolved_name,
                resolve_tileset_name(tileset_name_, tileset_names, text_formatter),
                void,
                "Pass '{}' to edit its config anyway.",
                FormatParam("--allow-missing-tileset", Style::bold));
            tileset_name = std::move(resolved_name);
        }

        const auto tileset_dir = project_root / "porytiles" / "tilesets" / tileset_name;
        const auto config_file = tileset_dir / (local_ ? "config.local.yaml" : "config.yaml");

        constexpr EditorLauncher launcher{};
        return launcher.create_and_edit_file(config_file);
    }

    static constexpr auto command_name = "edit-tileset-config";
    static constexpr auto command_desc = "Open a tileset's YAML config file in your editor.";
    static constexpr auto command_group = "UTILITIES";
    std::string tileset_name_;
    bool local_{false};
    bool allow_missing_tileset_{false};
    OptProjectRoot project_root_opt_;
};
