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
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to compile")->required();
        project_root_opt_.RegisterOpt(cmd);
        porytiles::register_config_options(cmd, cli_storage_);
    }

  private:
    void Run() override
    {
        using namespace porytiles;

        TilesetCommandEnv env{project_root_opt_.project_root(), tileset_name_, cli_storage_};
        TilesetCommandServices services{env, tileset_name_};

        // Verify the tileset exists in the project before proceeding
        if (!services.metadata_provider.exists(tileset_name_)) {
            const auto not_found_err = ChainableResult<void>{FormattableError{
                "Tileset '{}' does not exist. Create or import it first.", FormatParam{tileset_name_, Style::bold}}};
            env.diag->fatal(not_found_err);
            throw CLI::RuntimeError{1};
        }

        // Detect primary vs secondary and dispatch to the correct use case
        auto is_secondary_result = services.metadata_provider.is_secondary(tileset_name_);
        if (!is_secondary_result.has_value()) {
            env.diag->fatal(is_secondary_result);
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
            compile_result = compile_use_case.compile(tileset_name_);
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
            compile_result = compile_use_case.compile(tileset_name_);
        }
        if (!compile_result.has_value()) {
            const auto fail_result = ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to compile tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                compile_result};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
    }

    static constexpr auto kCommandName = "compile-tileset";
    static constexpr auto kCommandDesc =
        "Compile a tileset -- update the Porymap assets to match the Porytiles assets.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
