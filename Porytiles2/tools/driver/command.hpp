#pragma once

#include <string>

#include "CLI/CLI.hpp"

#include "porytiles2/infra/config/lazy_layered_config.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

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

    Command(CLI::App &parent_app, const std::string &name, const std::string &desc, const std::string &group)
        : app_(nullptr)
    {
        if (name.empty()) {
            porytiles2::panic("Command name cannot be empty.");
        }

        app_ = parent_app.add_subcommand(name, desc);
        porytiles2::assert_or_panic(app_ != nullptr, "CLI::App::add_subcommand returned nullptr for: " + name);

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

    [[nodiscard]] CLI::App &get_app() const
    {
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
