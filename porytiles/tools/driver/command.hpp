#pragma once

#include <string>

#include "CLI/CLI.hpp"

#include "porytiles/infra/config/lazy_layered_config.hpp"
#include "porytiles/utilities/panic/panic.hpp"

/// @brief Command is an abstract class that provides basic command functionality for the Porytiles
/// CLI driver.
///
/// @details
/// Command is an abstract class that provides basic command functionality for the Porytiles CLI
/// driver.
class Command {
  public:
    virtual ~Command() = default;

    Command(CLI::App &parent_app, const std::string &name, const std::string &desc, const std::string &group)
        : app_(nullptr)
    {
        if (name.empty()) {
            porytiles::panic("Command name cannot be empty.");
        }

        app_ = parent_app.add_subcommand(name, desc);
        porytiles::assert_or_panic(app_ != nullptr, "CLI::App::add_subcommand returned nullptr for: " + name);

        // In CLI11, setting group to empty string hides the command from help output
        app_->group(group);

        app_->callback([this] { this->Run(); });
    }

    // Prevent copy/move semantics
    Command(const Command &) = delete;
    Command &operator=(const Command &) = delete;
    Command(Command &&) = delete;
    Command &operator=(Command &&) = delete;

    [[nodiscard]] CLI::App &get_app() const
    {
        if (app_ == nullptr) {
            porytiles::panic("app_ should have been initialized by the constructor");
        }
        return *app_;
    }

  private:
    /// @brief The command's entry point, invoked through the CLI11 callback registered by the constructor.
    ///
    /// @details
    /// Deliberately private, here and in the overrides: Run() assumes CLI11 parsing already populated the command's
    /// option storage, so nothing outside the parse flow should be able to call it.
    virtual void Run() = 0;

    CLI::App *app_;
};
