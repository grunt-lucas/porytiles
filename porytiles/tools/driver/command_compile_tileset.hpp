#pragma once

#include <memory>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles/app/use_cases/compile_primary_tileset.hpp"
#include "porytiles/app/use_cases/compile_secondary_tileset.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

#include "command.hpp"
#include "option.hpp"
#include "tileset_command_setup.hpp"

class CompileTilesetCommand final : public Command {
  public:
    explicit CompileTilesetCommand(CLI::App &parent_app)
        : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to compile (fuzzy names accepted)")
            ->required();
        project_root_opt_.RegisterOpt(cmd);
        porytiles::register_config_options(cmd, cli_storage_);
    }

  private:
    void Run() override
    {
        using namespace porytiles;

        TilesetCommandEnv env{project_root_opt_.project_root(), cli_storage_};

        // The fuzzy name argument must be resolved to its canonical form first, since the canonical name is the config
        // scope for env initialization and the key for every later lookup. This resolution also guarantees the tileset
        // existence.
        auto resolved_name_result = resolve_tileset_name_argument(env, tileset_name_);
        if (!resolved_name_result.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to compile tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                resolved_name_result};
            env.stderr_diag.fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
        const std::string tileset_name = std::move(resolved_name_result).value();

        // Env initialization and schema resolution can fail, so they run first and their failures chain and report
        // through the unfiltered stderr diagnostics.
        const auto env_result = env.initialize(tileset_name);
        if (!env_result.has_value()) {
            const auto env_fail_result = ChainableResult<void>{
                FormattableError{"Failed to compile tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                env_result};
            env.stderr_diag.fatal(env_fail_result);
            throw CLI::RuntimeError{1};
        }

        auto attribute_context = resolve_attribute_context(env, tileset_name);
        if (!attribute_context.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to compile tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                attribute_context};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
        TilesetCommandServices services{env, std::move(attribute_context).value()};

        // Detect primary vs secondary and dispatch to the correct use case
        auto is_secondary_result = services.metadata_provider.is_secondary(tileset_name);
        if (!is_secondary_result.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to compile tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                is_secondary_result};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }

        ChainableResult<void> compile_result;
        if (is_secondary_result.value()) {
            CompileSecondaryTileset compile_use_case{
                &services.repo,
                &services.compiler,
                &services.metadata_provider,
                &services.layout_metadata_provider,
                &services.tileset_manager,
                &env.config,
                &env.config,
                env.diag.get()};
            compile_result = compile_use_case.compile(tileset_name);
        }
        else {
            CompilePrimaryTileset compile_use_case{
                &services.repo,
                &services.compiler,
                &services.metadata_provider,
                &services.tileset_manager,
                &env.config,
                &env.config,
                env.diag.get()};
            compile_result = compile_use_case.compile(tileset_name);
        }
        if (!compile_result.has_value()) {
            const auto fail_result = ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to compile tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                compile_result};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
    }

    static constexpr auto command_name = "compile-tileset";
    static constexpr auto command_desc =
        "Compile a tileset -- update the Porymap assets to match the Porytiles assets.";
    static constexpr auto command_group = "COMMANDS";
    std::string tileset_name_;
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
