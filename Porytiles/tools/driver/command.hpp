#pragma once

#include <CLI/CLI.hpp>
#include <iostream>
#include <porytiles/panic/panic.hpp>
#include <string>

#include "./option_group.hpp"

/// Collection of all options for 'compile-primary'.
struct CompilePrimaryOpts {
    OptGroupFieldmap fieldmapOpts;
};

class Command {
    CLI::App *command_;

  protected:
    virtual void Run() = 0;

    [[nodiscard]] CLI::App &get_command() const {
        if (command_ == nullptr) {
            porytiles::panic("command_ should have been initialized by the constructor");
        }
        return *command_;
    }

  public:
    virtual ~Command() = default;

    Command(CLI::App &parent_app, const std::string &name, const std::string &desc, const std::string &group)
        : command_(nullptr) {
        if (name.empty()) {
            porytiles::panic("Command name cannot be empty.");
        }

        command_ = parent_app.add_subcommand(name, desc);
        if (command_ == nullptr) {
            // Should not happen if CLI11 is working, but good practice
            porytiles::panic("CLI::App::add_subcommand returned nullptr for: " + name);
        }

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
};

class CompilePrimaryCommand final : public Command {
    static constexpr auto kCommandName = "compile-primary";
    static constexpr auto kCommandDesc = "Compile a primary tileset using explicit asset paths.";
    static constexpr auto kCommandGroup = "LEGACY COMMANDS";

    OptGroupFieldmap fieldmap_opts_;

  public:
    explicit CompilePrimaryCommand(CLI::App &parent_app)
        : Command(parent_app, kCommandName, kCommandDesc, kCommandGroup) {
        CLI::App &cmd = get_command();
        fieldmap_opts_.RegisterOptions(cmd);
    }

    void Run() override {
        std::cout << "Compile primary command called." << std::endl;
    }
};

class CompileSecondaryCommand final : public Command {
    static constexpr auto kCommandName = "compile-secondary";
    static constexpr auto kCommandDesc = "Compile a secondary tileset using explicit asset paths.";
    static constexpr auto kCommandGroup = "LEGACY COMMANDS";

    OptGroupFieldmap fieldmap_opts_;

  public:
    explicit CompileSecondaryCommand(CLI::App &parent_app)
        : Command(parent_app, kCommandName, kCommandDesc, kCommandGroup) {
        CLI::App &cmd = get_command();
        fieldmap_opts_.RegisterOptions(cmd);
    }

    void Run() override {
        std::cout << "Compile secondary command called." << std::endl;
    }
};

class DecompilePrimaryCommand final : public Command {
    static constexpr auto kCommandName = "decompile-primary";
    static constexpr auto kCommandDesc = "Decompile a primary tileset using explicit asset paths.";
    static constexpr auto kCommandGroup = "LEGACY COMMANDS";

    OptGroupFieldmap fieldmap_opts_;

  public:
    explicit DecompilePrimaryCommand(CLI::App &parent_app)
        : Command(parent_app, kCommandName, kCommandDesc, kCommandGroup) {
        CLI::App &cmd = get_command();
        fieldmap_opts_.RegisterOptions(cmd);
    }

    void Run() override {
        std::cout << "Decompile primary command called." << std::endl;
    }
};

class DecompileSecondaryCommand final : public Command {
    static constexpr auto kCommandName = "decompile-secondary";
    static constexpr auto kCommandDesc = "Decompile a secondary tileset using explicit asset paths.";
    static constexpr auto kCommandGroup = "LEGACY COMMANDS";

    OptGroupFieldmap fieldmap_opts_;

  public:
    explicit DecompileSecondaryCommand(CLI::App &parent_app)
        : Command(parent_app, kCommandName, kCommandDesc, kCommandGroup) {
        CLI::App &cmd = get_command();
        fieldmap_opts_.RegisterOptions(cmd);
    }

    void Run() override {
        std::cout << "Decompile secondary command called." << std::endl;
    }
};
