#pragma once

#include <CLI/CLI.hpp>
#include <iostream>
#include <porytiles/panic/panic.hpp>
#include <string>

#include "./option_group.hpp"

/// @brief Command is an abstract class that provides basic command
/// functionality for the Porytiles CLI driver.
class Command {
    CLI::App *command_;

  protected:
    virtual void Run() = 0;

  public:
    virtual ~Command() = default;

    Command(CLI::App &parent_app, const std::string &name, const std::string &desc, const std::string &group)
        : command_(nullptr) {
        if (name.empty()) {
            porytiles::panic("Command name cannot be empty.");
        }

        command_ = parent_app.add_subcommand(name, desc);
        porytiles::assert_or_panic(command_ != nullptr, "CLI::App::add_subcommand returned nullptr for: " + name);

        if (!group.empty()) {
            command_->group(group);
        }

        command_->callback([this] { this->Run(); });
    }

    // Prevent copy/move semantics
    Command(const Command &) = delete;
    Command &operator=(const Command &) = delete;
    Command(Command &&) = delete;
    Command &operator=(Command &&) = delete;

    [[nodiscard]] CLI::App &get_command() const {
        if (command_ == nullptr) {
            porytiles::panic("command_ should have been initialized by the constructor");
        }
        return *command_;
    }
};

class CompilePrimaryCommand final : public Command {
    static constexpr auto kCommandName = "compile-primary";
    static constexpr auto kCommandDesc = "Compile a primary tileset using explicit asset paths.";
    static constexpr auto kCommandGroup = "LEGACY COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;

  public:
    explicit CompilePrimaryCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {
        CLI::App &cmd = get_command();
        fieldmap_opts_.RegisterOptions(cmd);
        diagnostics_opts_.RegisterOptions(cmd);
    }

    void Run() override {
        std::cout << "Legacy compile primary command called." << std::endl;
        for (const auto &option : diagnostics_opts_.diagnostics_) {
            std::cout << option << std::endl;
        }
    }
};

class CompileSecondaryCommand final : public Command {
    static constexpr auto kCommandName = "compile-secondary";
    static constexpr auto kCommandDesc = "Compile a secondary tileset using explicit asset paths.";
    static constexpr auto kCommandGroup = "LEGACY COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;

  public:
    explicit CompileSecondaryCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {
        CLI::App &cmd = get_command();
        fieldmap_opts_.RegisterOptions(cmd);
        diagnostics_opts_.RegisterOptions(cmd);
    }

    void Run() override {
        std::cout << "Legacy compile secondary command called." << std::endl;
    }
};

class DecompilePrimaryCommand final : public Command {
    static constexpr auto kCommandName = "decompile-primary";
    static constexpr auto kCommandDesc = "Decompile a primary tileset using explicit asset paths.";
    static constexpr auto kCommandGroup = "LEGACY COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;

  public:
    explicit DecompilePrimaryCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {
        CLI::App &cmd = get_command();
        fieldmap_opts_.RegisterOptions(cmd);
        diagnostics_opts_.RegisterOptions(cmd);
    }

    void Run() override {
        std::cout << "Legacy decompile primary command called." << std::endl;
    }
};

class DecompileSecondaryCommand final : public Command {
    static constexpr auto kCommandName = "decompile-secondary";
    static constexpr auto kCommandDesc = "Decompile a secondary tileset using explicit asset paths.";
    static constexpr auto kCommandGroup = "LEGACY COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;

  public:
    explicit DecompileSecondaryCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {
        CLI::App &cmd = get_command();
        fieldmap_opts_.RegisterOptions(cmd);
        diagnostics_opts_.RegisterOptions(cmd);
    }

    void Run() override {
        std::cout << "Legacy decompile secondary command called." << std::endl;
    }
};

class CompileCommand final : public Command {
    static constexpr auto kCommandName = "compile";
    static constexpr auto kCommandDesc = "Compile Porytiles-format assets to Porymap-format assets.";
    static constexpr auto kCommandGroup = "COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;

  public:
    explicit CompileCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {
        CLI::App &cmd = get_command();
        fieldmap_opts_.RegisterOptions(cmd);
        diagnostics_opts_.RegisterOptions(cmd);
    }

    void Run() override {}
};

class CompileTilesetCommand final : public Command {
    static constexpr auto kCommandName = "tileset";
    static constexpr auto kCommandDesc = "Compile a Porytiles-format tileset to a Porymap-format tileset.";
    static constexpr auto kCommandGroup = "COMMANDS";

  public:
    explicit CompileTilesetCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {}

    void Run() override {
        std::cout << "Compile tileset command called." << std::endl;
    }
};

class CompileLayoutCommand final : public Command {
    static constexpr auto kCommandName = "layout";
    static constexpr auto kCommandDesc = "Compile a Porytiles-format layout to a Porymap-format layout.";
    static constexpr auto kCommandGroup = "COMMANDS";

  public:
    explicit CompileLayoutCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {}

    void Run() override {
        std::cout << "Compile layout command called." << std::endl;
    }
};

class CompileSpritesheetCommand final : public Command {
    static constexpr auto kCommandName = "spritesheet";
    static constexpr auto kCommandDesc = "Compile a Porytiles-format spritesheet to a Porymap-format spritesheet.";
    static constexpr auto kCommandGroup = "COMMANDS";

  public:
    explicit CompileSpritesheetCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {}

    void Run() override {
        std::cout << "Compile spritesheet command called." << std::endl;
    }
};

class DecompileCommand final : public Command {
    static constexpr auto kCommandName = "decompile";
    static constexpr auto kCommandDesc = "Decompile Porymap-format assets back to Porytiles-format assets.";
    static constexpr auto kCommandGroup = "COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;

  public:
    explicit DecompileCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {
        CLI::App &cmd = get_command();
        fieldmap_opts_.RegisterOptions(cmd);
        diagnostics_opts_.RegisterOptions(cmd);
    }

    void Run() override {}
};

class DecompileTilesetCommand final : public Command {
    static constexpr auto kCommandName = "tileset";
    static constexpr auto kCommandDesc = "Decompile a Porymap-format tileset back to a Porytiles-format tileset.";
    static constexpr auto kCommandGroup = "COMMANDS";

  public:
    explicit DecompileTilesetCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {}

    void Run() override {
        std::cout << "Decompile tileset command called." << std::endl;
    }
};

class DecompileLayoutCommand final : public Command {
    static constexpr auto kCommandName = "layout";
    static constexpr auto kCommandDesc = "Decompile a Porymap-format layout back to a Porytiles-format layout.";
    static constexpr auto kCommandGroup = "COMMANDS";

  public:
    explicit DecompileLayoutCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {}

    void Run() override {
        std::cout << "Decompile layout command called." << std::endl;
    }
};

class ReduceBitDepthCommand final : public Command {
    static constexpr auto kCommandName = "reduce-bit-depth";
    static constexpr auto kCommandDesc = "Reduce bit depth for given input assets.";
    static constexpr auto kCommandGroup = "COMMANDS";

  public:
    explicit ReduceBitDepthCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {}

    void Run() override {
        std::cout << "Reduce bit depth command called." << std::endl;
    }
};
