#pragma once

#include <iostream>

#include "CLI/CLI.hpp"

#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

#include "command.hpp"
#include "option.hpp"
#include "tileset_command_setup.hpp"

/// @brief Dumps the project configuration provenance chain.
///
/// @details
/// DumpProjectConfigCommand is the project-scope cousin of DumpTilesetConfigCommand: it resolves the same provider
/// chain (CLI options, project-wide YAML files, base-game header defines, defaults) but without any tileset-level
/// YAML files. Useful for seeing the project-wide defaults every tileset inherits, without resolving a specific
/// tileset.
class DumpProjectConfigCommand final : public Command {
  public:
    explicit DumpProjectConfigCommand(CLI::App &parent_app)
        : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        project_root_opt_.RegisterOpt(cmd);
        porytiles::register_config_options(cmd, cli_storage_);
    }

  private:
    void Run() override
    {
        using namespace porytiles;

        TilesetCommandEnv env{project_root_opt_.project_root(), cli_storage_};

        // Env failures report through the unfiltered stderr diagnostics: the filtered diagnostics handle is not built
        // until initialize_project() succeeds.
        const auto env_result = env.initialize_project();
        if (!env_result.has_value()) {
            const auto env_fail_result =
                ChainableResult<void>{FormattableError{"Failed to dump the project config."}, env_result};
            env.stderr_diag.fatal(env_fail_result);
            throw CLI::RuntimeError{1};
        }

        env.config.dump_config(std::cout, ConfigScopeType::project, "");
    }

    static constexpr auto command_name = "dump-project-config";
    static constexpr auto command_desc = "Dump the project configuration provenance chain.";
    static constexpr auto command_group = "UTILITIES";
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
