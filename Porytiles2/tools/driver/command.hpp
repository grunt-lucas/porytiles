#pragma once

#include <iostream>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/infra/config/default_provider.hpp"
#include "porytiles2/infra/config/lazy_layered_config.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_writer.hpp"
#include "porytiles2/infra/services/jasc_pal_loader.hpp"
#include "porytiles2/infra/services/jasc_pal_saver.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/infra/services/project_artifact_checksum_provider.hpp"
#include "porytiles2/templates/panic.hpp"
#include "porytiles2/templates/result.hpp"
#include "porytiles2/templates/text_formatter.hpp"

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

class CreateLayoutCommand final : public Command {
  public:
    explicit CreateLayoutCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        fieldmap_opts_.RegisterGroup(cmd);
        diagnostics_opts_.RegisterGroup(cmd);
    }

    void Run() override
    {
        std::cout << "Create layout command called." << std::endl;
    }

  private:
    static constexpr auto kCommandName = "create-layout";
    static constexpr auto kCommandDesc = "Create a new layout.";
    static constexpr auto kCommandGroup = "COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;
};

class CompileTilesetCommand final : public Command {
  public:
    explicit CompileTilesetCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        fieldmap_opts_.RegisterGroup(cmd);
        diagnostics_opts_.RegisterGroup(cmd);
    }

    void Run() override
    {
        std::cout << "Compile tileset command called." << std::endl;
    }

  private:
    static constexpr auto kCommandName = "compile-tileset";
    static constexpr auto kCommandDesc = "Compile a tileset's Porytiles component.";
    static constexpr auto kCommandGroup = "COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;
};

class CompileLayoutCommand final : public Command {
  public:
    explicit CompileLayoutCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        fieldmap_opts_.RegisterGroup(cmd);
        diagnostics_opts_.RegisterGroup(cmd);
    }

    void Run() override
    {
        std::cout << "Compile layout command called." << std::endl;
    }

  private:
    static constexpr auto kCommandName = "compile-layout";
    static constexpr auto kCommandDesc = "Compile a layout's Porytiles component.";
    static constexpr auto kCommandGroup = "COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;
};

class ImportTilesetCommand final : public Command {
  public:
    explicit ImportTilesetCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        fieldmap_opts_.RegisterGroup(cmd);
        diagnostics_opts_.RegisterGroup(cmd);
    }

    void Run() override
    {
        std::cout << "Import tileset command called." << std::endl;
    }

  private:
    static constexpr auto kCommandName = "import-tileset";
    static constexpr auto kCommandDesc = "Import a tileset's Porymap component.";
    static constexpr auto kCommandGroup = "COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;
};

class ImportLayoutCommand final : public Command {
  public:
    explicit ImportLayoutCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        fieldmap_opts_.RegisterGroup(cmd);
        diagnostics_opts_.RegisterGroup(cmd);
    }

    void Run() override
    {
        std::cout << "Import layout command called." << std::endl;
    }

  private:
    static constexpr auto kCommandName = "import-layout";
    static constexpr auto kCommandDesc = "Import a layout's Porymap component.";
    static constexpr auto kCommandGroup = "COMMANDS";

    OptGroupFieldmap fieldmap_opts_;
    OptGroupDiagnostics diagnostics_opts_;
};

class ReduceBitDepthCommand final : public Command {
  public:
    explicit ReduceBitDepthCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
    }

    void Run() override
    {
        std::cout << "Reduce bit depth command called." << std::endl;
    }

  private:
    static constexpr auto kCommandName = "reduce-bit-depth";
    static constexpr auto kCommandDesc = "Reduce bit depth for given input assets.";
    static constexpr auto kCommandGroup = "COMMANDS";
};

class DebugCommand final : public Command {
  public:
    explicit DebugCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to load")->required();
    }

    void Run() override
    {
        using namespace porytiles2;

        // Initialize stateless services
        PngRgbaImageLoader png_rgba_loader{};
        PngIndexedImageLoader png_indexed_loader{};
        PngRgbaImageSaver png_rgba_saver{};
        PngIndexedImageSaver png_indexed_saver{};
        JascPalLoader jasc_loader{};
        JascPalSaver jasc_saver{};
        TextFormatter formatter{true};

        // Setup layered configuration
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<DefaultProvider>());
        LazyLayeredConfig config{std::move(providers)};

        // Setup the tileset repository
        ProjectTilesetArtifactReader artifact_reader{&png_rgba_loader, &png_indexed_loader, &jasc_loader};
        ProjectTilesetArtifactWriter artifact_writer{&config, ".", &png_rgba_saver, &png_indexed_saver, &jasc_saver};
        ProjectTilesetArtifactKeyProvider key_provider{"."};
        ProjectArtifactChecksumProvider checksum_provider{&key_provider};
        TilesetRepo repo{&checksum_provider, &key_provider, &artifact_reader, &artifact_writer};

        // Command logic
        const auto load_result = repo.load(tileset_name_);
        if (!load_result.has_value()) {
            for (const auto &err : load_result.chain()) {
                std::cerr << err->details(formatter) << std::endl;
            }
        }
        else {
            const auto save_result = repo.save(*load_result.value());
            if (!save_result.has_value()) {
                std::cout << "save error: " << save_result.error() << std::endl;
            }
        }
    }

  private:
    static constexpr auto kCommandName = "debug";
    static constexpr auto kCommandDesc = "Stub command for development testing of Porytiles2 components.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
};
