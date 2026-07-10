#pragma once

#include <string>

#include "CLI/CLI.hpp"

#include "porytiles/app/use_cases/create_primary_tileset.hpp"
#include "porytiles/app/use_cases/create_secondary_tileset.hpp"
#include "porytiles/domain/services/tileset_creator.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

#include "command.hpp"
#include "option.hpp"
#include "tileset_command_setup.hpp"

class CreateTilesetCommand final : public Command {
  public:
    explicit CreateTilesetCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to create (e.g., gTileset_MyTileset)")
            ->required();
        cmd.add_flag("--secondary", secondary_, "Create a secondary tileset instead of a primary.");
        project_root_opt_.RegisterOpt(cmd);
        porytiles::register_config_options(cmd, cli_storage_);
    }

    void Run() override
    {
        using namespace porytiles;

        TilesetCommandEnv env{project_root_opt_.project_root(), tileset_name_, cli_storage_};
        TilesetCommandServices services{env, tileset_name_};

        // Setup creator. The creator seeds its sample art with a behavior constant name, so it needs the
        // behavior field's provider. A schema without a provider-backed behavior field cannot support creation, so fail
        // fast here rather than letting the creator resolve names against nothing.
        const auto behavior_provider_it = services.provider_map.find(attr::field_behavior);
        if (behavior_provider_it == services.provider_map.end()) {
            const auto no_behavior_err = ChainableResult<void>{FormattableError{
                "Cannot create a tileset: the resolved attribute schema has no provider-backed '{}' field.",
                FormatParam{std::string{attr::field_behavior}, Style::bold}}};
            env.diag->fatal(no_behavior_err);
            throw CLI::RuntimeError{1};
        }
        TilesetCreator creator{&env.config, behavior_provider_it->second.get()};

        // Create and run the appropriate use case based on --secondary flag
        ChainableResult<void> create_result;
        if (secondary_) {
            CreateSecondaryTileset create_use_case{
                &creator,
                &services.compiler,
                &services.repo,
                &services.metadata_provider,
                &services.layout_metadata_provider,
                &services.tileset_manager,
                &env.config,
                &env.config,
                env.diag.get()};
            create_result = create_use_case.create(tileset_name_);
        }
        else {
            CreatePrimaryTileset create_use_case{
                &creator,
                &services.compiler,
                &services.repo,
                &services.metadata_provider,
                &services.tileset_manager,
                &env.config,
                &env.config,
                env.diag.get()};
            create_result = create_use_case.create(tileset_name_);
        }
        if (!create_result.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to create tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                create_result};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
    }

  private:
    static constexpr auto kCommandName = "create-tileset";
    static constexpr auto kCommandDesc = "Create a new Porytiles-managed tileset from scratch.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
    bool secondary_ = false;
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
