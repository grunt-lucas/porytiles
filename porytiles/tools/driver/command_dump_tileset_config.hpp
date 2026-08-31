#pragma once

#include <iostream>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

#include "command.hpp"
#include "option.hpp"
#include "tileset_command_setup.hpp"

class DumpTilesetConfigCommand final : public Command {
  public:
    explicit DumpTilesetConfigCommand(CLI::App &parent_app)
        : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to dump config for (fuzzy names accepted)")
            ->required();
        cmd.add_flag(
            "--allow-missing-tileset",
            allow_missing_tileset_,
            "Dump config even when the tileset does not exist, to preview what a new tileset would inherit.");
        project_root_opt_.RegisterOpt(cmd);
        porytiles::register_config_options(cmd, cli_storage_);
    }

  private:
    void Run() override
    {
        using namespace porytiles;

        TilesetCommandEnv env{project_root_opt_.project_root(), cli_storage_};

        // The fuzzy name argument resolves to its canonical form before env initialization, since the canonical name
        // is the config scope. Resolution also replaces the old existence check: a misspelled name would otherwise
        // dump a plausible-looking chain of project-level values. --allow-missing-tileset skips resolution (there is
        // no declared name to resolve against) and uses the argument verbatim, for previewing the config a
        // not-yet-created tileset would inherit.
        std::string tileset_name = tileset_name_;
        if (!allow_missing_tileset_) {
            auto resolved_name_result = resolve_tileset_name_argument(env, tileset_name_);
            if (!resolved_name_result.has_value()) {
                const auto fail_result = ChainableResult<void>{
                    FormattableError{
                        "Failed to dump config for tileset '{}'. Pass '{}' to dump its config anyway.",
                        FormatParam{tileset_name_, Style::bold},
                        FormatParam{"--allow-missing-tileset", Style::bold}},
                    resolved_name_result};
                env.stderr_diag.fatal(fail_result);
                throw CLI::RuntimeError{1};
            }
            tileset_name = std::move(resolved_name_result).value();
        }

        // Env failures report through the unfiltered stderr diagnostics: the filtered diagnostics handle is not built
        // until initialize() succeeds.
        const auto env_result = env.initialize(tileset_name);
        if (!env_result.has_value()) {
            const auto env_fail_result = ChainableResult<void>{
                FormattableError{"Failed to dump config for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                env_result};
            env.stderr_diag.fatal(env_fail_result);
            throw CLI::RuntimeError{1};
        }

        env.config.dump_config(std::cout, ConfigScopeType::tileset, tileset_name);
    }

    static constexpr auto command_name = "dump-tileset-config";
    static constexpr auto command_desc = "Dump the full configuration provenance chain for a tileset.";
    static constexpr auto command_group = "UTILITIES";
    std::string tileset_name_;
    bool allow_missing_tileset_{false};
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
