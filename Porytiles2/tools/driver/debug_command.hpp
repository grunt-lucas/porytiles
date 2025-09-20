#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles2/domain/compile_tileset/primary_tileset_compiler.hpp"
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
#include "porytiles2/templates/result.hpp"
#include "porytiles2/templates/text_formatter.hpp"

#include "command.hpp"

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
        PrimaryTilesetCompiler compiler{};

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

        // Run compilation
        auto maybe_tileset = repo.load(tileset_name_);
        if (!maybe_tileset.has_value()) {
            for (const auto &err : maybe_tileset.chain()) {
                std::cerr << err->details(formatter) << std::endl;
            }
        }
        const auto tileset = std::move(maybe_tileset.value());
        const auto maybe_porymap = compiler.compile(tileset->porytiles_component());
        if (!maybe_porymap.has_value()) {
            std::cerr << maybe_porymap.error().details(TextFormatter{false}) << std::endl;
        }
    }

  private:
    static constexpr auto kCommandName = "debug";
    static constexpr auto kCommandDesc = "Stub hook for development testing of Porytiles2 components.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
};
