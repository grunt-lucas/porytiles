#pragma once

#include <filesystem>

#include "CLI/CLI.hpp"

#include "validators.hpp"

class Opt {
  public:
    virtual ~Opt() = default;

    [[nodiscard]] virtual std::string NameShort() const = 0;

    [[nodiscard]] virtual std::string NameLong() const = 0;

    virtual void RegisterOpt(CLI::App &app) = 0;

    void SetGroup(const std::string &group, CLI::App &app) const
    {
        app.get_option(NameLong())->group(group);
    }

    [[nodiscard]] std::string NameCombined() const
    {
        return NameShort() + "," + NameLong();
    }
};

class OptProjectRoot final : public Opt {
    std::string project_root_;

  public:
    OptProjectRoot() : project_root_{"."} {}

    [[nodiscard]] std::string NameShort() const override
    {
        return "-C";
    }

    [[nodiscard]] std::string NameLong() const override
    {
        return "--project-root";
    }

    void RegisterOpt(CLI::App &app) override
    {
        app.add_option(
               NameCombined(),
               project_root_,
               "Set the project root directory. Porytiles will look for "
               "porytiles management directory, include/fieldmap.h, tilesets, "
               "and other project files relative to this path.")
            ->check(CLI::ExistingDirectory)
            ->capture_default_str();
    }

    [[nodiscard]] std::filesystem::path project_root() const
    {
        return std::filesystem::path{project_root_};
    }
};
