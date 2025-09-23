#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/domain/services/rgba_layer_image_metatileizer.hpp"
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
#include "porytiles2/domain/services/rgba_layer_image_metatileizer.hpp"

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

        // Load the tileset
        auto maybe_tileset = repo.load(tileset_name_);
        if (!maybe_tileset.has_value()) {
            for (const auto &err : maybe_tileset.chain()) {
                std::cerr << err->details(formatter) << std::endl;
            }
        }
        const auto tileset = std::move(maybe_tileset.value());

        // Normalize tiles and write them back so we can inspect the normalization results
        RgbaLayerImageMetatileizer metatileizer{};
        auto maybe_metatiles = metatileizer.metatileize(
            tileset->porytiles_component().bottom(),
            tileset->porytiles_component().middle(),
            tileset->porytiles_component().top());
        if (!maybe_metatiles.has_value()) {
            for (const auto &err : maybe_metatiles.chain()) {
                std::cerr << err->details(formatter) << std::endl;
            }
        }
        const auto metatiles = std::move(maybe_metatiles.value());

        std::vector<RgbaMetatile> new_metatiles{};
        for (const auto &metatile : metatiles) {
        }
    }

  private:
    static constexpr auto kCommandName = "debug";
    static constexpr auto kCommandDesc = "Stub hook for development testing of Porytiles2 components.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
};
