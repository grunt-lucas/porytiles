#pragma once

#include <memory>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles/app/use_cases/import_primary_tileset.hpp"
#include "porytiles/domain/services/primary_tileset_decompiler.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/infra/services/project_primary_tileset_importer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

#include "command.hpp"
#include "option.hpp"
#include "tileset_command_setup.hpp"

class ImportTilesetCommand final : public Command {
  public:
    explicit ImportTilesetCommand(CLI::App &parent_app) : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to import")->required();
        project_root_opt_.RegisterOpt(cmd);
        porytiles::register_config_options(cmd, cli_storage_);
    }

  private:
    void Run() override
    {
        using namespace porytiles;

        TilesetCommandEnv env{project_root_opt_.project_root(), cli_storage_};

        // Env initialization and schema resolution can fail, so they run first and their failures chain and report
        // through the unfiltered stderr diagnostics.
        const auto env_result = env.initialize(tileset_name_);
        if (!env_result.has_value()) {
            const auto env_fail_result = ChainableResult<void>{
                FormattableError{"Failed to import tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                env_result};
            env.stderr_diag.fatal(env_fail_result);
            throw CLI::RuntimeError{1};
        }

        auto attribute_context = resolve_attribute_context(env, tileset_name_);
        if (!attribute_context.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to import tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                attribute_context};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
        TilesetCommandServices services{env, std::move(attribute_context).value()};

        ProjectPrimaryTilesetImporter importer{
            env.project_root,
            &services.resolved.schema,
            &env.config,
            env.text_formatter,
            env.diag.get(),
            services.tile_printer.get(),
            services.palette_printer.get(),
            &services.metadata_provider,
            &services.png_indexed_loader,
            &services.jasc_loader,
        };
        PrimaryTilesetDecompiler decompiler{
            &env.config,
            env.text_formatter,
            env.diag.get(),
            services.tile_printer.get(),
            services.palette_printer.get()};
        ImportPrimaryTileset import_use_case{
            &importer,
            &decompiler,
            &services.compiler,
            &services.repo,
            &services.metadata_provider,
            &services.tileset_manager,
            &env.config,
            &env.config,
            env.diag.get()};

        // Run the use case
        auto import_result = import_use_case.import(tileset_name_);
        if (!import_result.has_value()) {
            const auto fail_result = ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to import tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                import_result};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
    }

    static constexpr auto command_name = "import-tileset";
    static constexpr auto command_desc = "Import a pre-existing tileset into Porytiles.";
    static constexpr auto command_group = "COMMANDS";
    std::string tileset_name_;
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
