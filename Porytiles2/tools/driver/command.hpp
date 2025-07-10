#pragma once

#include <iostream>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles2/templates/panic.hpp"

#include "option.hpp"
#include "option_group.hpp"

/**
 * @brief Command is an abstract class that provides basic command functionality for the Porytiles
 * CLI driver.
 *
 * @details
 * Command is an abstract class that provides basic command functionality for the Porytiles CLI
 * driver.
 */
class Command {
public:
  virtual ~Command() = default;

  Command(CLI::App &parent_app, const std::string &name, const std::string &desc,
          const std::string &group)
      : app_(nullptr) {
    if (name.empty()) {
      porytiles2::panic("Command name cannot be empty.");
    }

    app_ = parent_app.add_subcommand(name, desc);
    porytiles2::assert_or_panic(app_ != nullptr,
                                "CLI::App::add_subcommand returned nullptr for: " + name);

    if (!group.empty()) {
      app_->group(group);
    }

    app_->callback([this] { this->Run(); });
  }

  // Prevent copy/move semantics
  Command(const Command &) = delete;
  Command &operator=(const Command &) = delete;
  Command(Command &&) = delete;
  Command &operator=(Command &&) = delete;

  [[nodiscard]] CLI::App &get_app() const {
    if (app_ == nullptr) {
      porytiles2::panic("app_ should have been initialized by the constructor");
    }
    return *app_;
  }

protected:
  virtual void Run() = 0;

private:
  CLI::App *app_;
};

class CompileTilesetCommand final : public Command {
public:
  explicit CompileTilesetCommand(CLI::App &parent_app)
      : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {
    CLI::App &cmd = get_app();
    fieldmap_opts_.RegisterGroup(cmd);
    diagnostics_opts_.RegisterGroup(cmd);
  }

  void Run() override { std::cout << "Compile tileset command called." << std::endl; }

private:
  static constexpr auto kCommandName = "compile-tileset";
  static constexpr auto kCommandDesc =
      "Compile a Porytiles-format tileset to a Porymap-format tileset.";
  static constexpr auto kCommandGroup = "COMMANDS";

  OptGroupFieldmap fieldmap_opts_;
  OptGroupDiagnostics diagnostics_opts_;
};

class CompileLayoutCommand final : public Command {
public:
  explicit CompileLayoutCommand(CLI::App &parent_app)
      : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {
    CLI::App &cmd = get_app();
    fieldmap_opts_.RegisterGroup(cmd);
    diagnostics_opts_.RegisterGroup(cmd);
  }

  void Run() override { std::cout << "Compile layout command called." << std::endl; }

private:
  static constexpr auto kCommandName = "compile-layout";
  static constexpr auto kCommandDesc =
      "Compile a Porytiles-format layout to a Porymap-format layout.";
  static constexpr auto kCommandGroup = "COMMANDS";

  OptGroupFieldmap fieldmap_opts_;
  OptGroupDiagnostics diagnostics_opts_;
};

class CompileSpritesheetCommand final : public Command {
public:
  explicit CompileSpritesheetCommand(CLI::App &parent_app)
      : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {
    CLI::App &cmd = get_app();
    fieldmap_opts_.RegisterGroup(cmd);
    diagnostics_opts_.RegisterGroup(cmd);
  }

  void Run() override { std::cout << "Compile spritesheet command called." << std::endl; }

private:
  static constexpr auto kCommandName = "compile-spritesheet";
  static constexpr auto kCommandDesc =
      "Compile a Porytiles-format spritesheet to a Porymap-format spritesheet.";
  static constexpr auto kCommandGroup = "COMMANDS";

  OptGroupFieldmap fieldmap_opts_;
  OptGroupDiagnostics diagnostics_opts_;
};

class DecompileTilesetCommand final : public Command {
public:
  explicit DecompileTilesetCommand(CLI::App &parent_app)
      : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {
    CLI::App &cmd = get_app();
    fieldmap_opts_.RegisterGroup(cmd);
    diagnostics_opts_.RegisterGroup(cmd);
  }

  void Run() override { std::cout << "Decompile tileset command called." << std::endl; }

private:
  static constexpr auto kCommandName = "decompile-tileset";
  static constexpr auto kCommandDesc =
      "Decompile a Porymap-format tileset back to a Porytiles-format tileset.";
  static constexpr auto kCommandGroup = "COMMANDS";

  OptGroupFieldmap fieldmap_opts_;
  OptGroupDiagnostics diagnostics_opts_;
};

class DecompileLayoutCommand final : public Command {
public:
  explicit DecompileLayoutCommand(CLI::App &parent_app)
      : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {
    CLI::App &cmd = get_app();
    fieldmap_opts_.RegisterGroup(cmd);
    diagnostics_opts_.RegisterGroup(cmd);
  }

  void Run() override { std::cout << "Decompile layout command called." << std::endl; }

private:
  static constexpr auto kCommandName = "decompile-layout";
  static constexpr auto kCommandDesc =
      "Decompile a Porymap-format layout back to a Porytiles-format layout.";
  static constexpr auto kCommandGroup = "COMMANDS";

  OptGroupFieldmap fieldmap_opts_;
  OptGroupDiagnostics diagnostics_opts_;
};

class ReduceBitDepthCommand final : public Command {
public:
  explicit ReduceBitDepthCommand(CLI::App &parent_app)
      : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup} {}

  void Run() override { std::cout << "Reduce bit depth command called." << std::endl; }

private:
  static constexpr auto kCommandName = "reduce-bit-depth";
  static constexpr auto kCommandDesc = "Reduce bit depth for given input assets.";
  static constexpr auto kCommandGroup = "COMMANDS";
};
