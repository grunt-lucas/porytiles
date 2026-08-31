#pragma once

#include <unistd.h>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles/infra/services/editor_launcher.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/terminal_width.hpp"
#include "porytiles/xcut/di/components.hpp"
#include "porytiles/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"

#include "command.hpp"
#include "option.hpp"

/// @brief Opens the project YAML config file in the user's preferred editor.
///
/// @details
/// EditProjectConfigCommand is the project-scope cousin of EditTilesetConfigCommand: it resolves the editor via the
/// EditorLauncher service and opens the project-wide @c porytiles/config.yaml (or @c porytiles/config.local.yaml with
/// @c --local). When the porytiles directory or the config file does not exist yet, the command creates them first.
///
/// Like EditTilesetConfigCommand, this command skips the TilesetCommandEnv initialization so a currently-invalid
/// config can still be opened and fixed.
class EditProjectConfigCommand final : public Command {
  public:
    explicit EditProjectConfigCommand(CLI::App &parent_app)
        : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        cmd.add_flag("--local", local_, "Edit the project 'config.local.yaml' instead of the shared 'config.yaml'.");
        project_root_opt_.RegisterOpt(cmd);
    }

  private:
    void Run() override
    {
        using namespace porytiles;

        fruit::Injector<TextFormatter> injector{di::get_formatter_component, !isatty(STDERR_FILENO)};
        auto *text_formatter = injector.get<TextFormatter *>();
        const StderrStyledUserDiagnostics diag{text_formatter, resolve_terminal_width(STDERR_FILENO)};

        const auto config_file =
            project_root_opt_.project_root() / "porytiles" / (local_ ? "config.local.yaml" : "config.yaml");

        constexpr EditorLauncher launcher{};
        const auto edit_result = launcher.create_and_edit_file(config_file);
        if (!edit_result.has_value()) {
            const auto fail_result =
                ChainableResult<void>{FormattableError{"Failed to edit the project config."}, edit_result};
            diag.fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
    }

    static constexpr auto command_name = "edit-project-config";
    static constexpr auto command_desc = "Open the project YAML config file in your editor.";
    static constexpr auto command_group = "UTILITIES";
    bool local_{false};
    OptProjectRoot project_root_opt_;
};
