#pragma once

#include <iostream>

#include "CLI/CLI.hpp"

#include "command.hpp"
#include "option_group.hpp"

class CreateTilesetCommand final : public Command {
  public:
    explicit CreateTilesetCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        fieldmap_opts_.RegisterGroup(cmd);
        diagnostics_opts_.RegisterGroup(cmd);
    }

    void Run() override
    {
        std::cout << "Create tileset command called." << std::endl;
    }

  private:
    static constexpr auto kCommandName = "create-tileset";
    static constexpr auto kCommandDesc = "Create a new tileset.";
    static constexpr auto kCommandGroup = "COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;
};
