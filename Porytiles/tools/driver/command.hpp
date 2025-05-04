#pragma once

#include <CLI/CLI.hpp>
#include <iostream>
#include <string>

#include "./option_group.hpp"

/// Collection of all options for 'compile-primary'.
struct CompilePrimaryOpts {
    OptGroupFieldmap fieldmapOpts;
};

class Command {
    CLI::App *command_;

  public:
    virtual ~Command() = default;

    Command() : command_(nullptr) {}

    virtual std::string CommandName() = 0;

    virtual std::string CommandDescription() = 0;

    virtual std::string CommandGroup() = 0;

    virtual void RegisterOptions() = 0;

    virtual void Setup(CLI::App &app) {
        command_ = app.add_subcommand(CommandName(), CommandDescription());
        command_->group(CommandGroup());
        command_->callback([this] { Run(); });
    }

    virtual void Run() = 0;

    [[nodiscard]] CLI::App &get_command() const {
        return *command_;
    }
};

class CompilePrimaryCommand final : public Command {
    OptGroupFieldmap fieldmap_opts_;

  public:
    CompilePrimaryCommand() = default;

    std::string CommandName() override {
        return "compile-primary";
    }

    std::string CommandDescription() override {
        return "Compile a primary tileset using explicit asset paths.";
    }

    std::string CommandGroup() override {
        return "LEGACY COMMANDS";
    }

    void RegisterOptions() override {
        fieldmap_opts_.RegisterOptions(get_command());
    }

    void Run() override {
        std::cout << "Running compile-primary!" << std::endl;
    }
};
