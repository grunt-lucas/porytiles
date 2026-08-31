#pragma once

#include <memory>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles/app/use_cases/decompile_primary_tileset.hpp"
#include "porytiles/domain/services/primary_tileset_decompiler.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

#include "command.hpp"
#include "option.hpp"
#include "tileset_command_setup.hpp"

class DecompileTilesetCommand final : public Command {
  public:
    explicit DecompileTilesetCommand(CLI::App &parent_app)
        : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to decompile (fuzzy names accepted)")
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
                FormattableError{"Failed to decompile tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
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
                FormattableError{"Failed to decompile tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                env_result};
            env.stderr_diag.fatal(env_fail_result);
            throw CLI::RuntimeError{1};
        }

        auto attribute_context = resolve_attribute_context(env, tileset_name);
        if (!attribute_context.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to decompile tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                attribute_context};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
        TilesetCommandServices services{env, std::move(attribute_context).value()};

        PrimaryTilesetDecompiler decompiler{
            &env.config,
            env.text_formatter,
            env.diag.get(),
            services.tile_printer.get(),
            services.palette_printer.get()};
        DecompilePrimaryTileset decompile_use_case{
            &services.repo,
            &decompiler,
            &services.compiler,
            &services.metadata_provider,
            &services.tileset_manager,
            &env.config,
            &env.config,
            env.diag.get()};

        // Run the use case
        auto decompile_result = decompile_use_case.decompile(tileset_name);
        if (!decompile_result.has_value()) {
            const auto fail_result = ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to decompile tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                decompile_result};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
    }

    static constexpr auto command_name = "decompile-tileset";
    static constexpr auto command_desc =
        "Decompile a tileset -- update the Porytiles assets to match the Porymap assets.";
    static constexpr auto command_group = "COMMANDS";
    std::string tileset_name_;
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
