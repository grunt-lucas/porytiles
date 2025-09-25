#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/domain/services/rgba_layer_image_metatileizer.hpp"
#include "porytiles2/domain/services/rgba_tile_normalizer.hpp"
#include "porytiles2/infra/config/default_provider.hpp"
#include "porytiles2/infra/config/lazy_layered_config.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_writer.hpp"
#include "porytiles2/infra/services/jasc_pal_loader.hpp"
#include "porytiles2/infra/services/jasc_pal_saver.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"
#include "porytiles2/infra/services/project_artifact_checksum_provider.hpp"
#include "porytiles2/templates/result.hpp"
#include "porytiles2/templates/text_formatter.hpp"

#include "command.hpp"

class DebugNormalizeCommand final : public Command {
  public:
    explicit DebugNormalizeCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to normalize")->required();
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
            return;
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
            return;
        }
        const auto metatiles = std::move(maybe_metatiles.value());

        std::vector<RgbaMetatile> new_metatiles{};
        for (const auto &metatile : metatiles) {
            RgbaTileNormalizer normalizer{};
            RgbaMetatile new_metatile{};

            // Normalize and denormalize tiles to test round-trip functionality
            std::size_t tile_idx = 0;
            for (const auto &bottom_tile : metatile.bottom()) {
                // Convert using the new conversion constructor
                RgbaTile rgba_tile{bottom_tile};
                auto normalized_result = normalizer.normalize(rgba_tile, rgba_magenta);
                if (normalized_result.has_value()) {
                    RgbaTile denormalized_tile = normalizer.denormalize_preserving_flips(normalized_result.value());
                    new_metatile.set_bottom(tile_idx, denormalized_tile);
                }
                ++tile_idx;
            }

            tile_idx = 0;
            for (const auto &middle_tile : metatile.middle()) {
                // Convert using the new conversion constructor
                RgbaTile rgba_tile{middle_tile};
                auto normalized_result = normalizer.normalize(rgba_tile, rgba_magenta);
                if (normalized_result.has_value()) {
                    RgbaTile denormalized_tile = normalizer.denormalize_preserving_flips(normalized_result.value());
                    new_metatile.set_middle(tile_idx, denormalized_tile);
                }
                ++tile_idx;
            }

            tile_idx = 0;
            for (const auto &top_tile : metatile.top()) {
                // Convert using the new conversion constructor
                RgbaTile rgba_tile{top_tile};
                auto normalized_result = normalizer.normalize(rgba_tile, rgba_magenta);
                if (normalized_result.has_value()) {
                    RgbaTile denormalized_tile = normalizer.denormalize_preserving_flips(normalized_result.value());
                    new_metatile.set_top(tile_idx, denormalized_tile);
                }
                ++tile_idx;
            }

            new_metatiles.push_back(std::move(new_metatile));
        }

        auto maybe_demetatileized_images = metatileizer.demetatileize(new_metatiles, 8);
        if (!maybe_demetatileized_images.has_value()) {
            for (const auto &err : maybe_demetatileized_images.chain()) {
                std::cerr << err->details(formatter) << std::endl;
            }
            return;
        }
        auto demetatileized_images = std::move(maybe_demetatileized_images.value());
        tileset->porytiles_component().bottom(std::get<0>(demetatileized_images));
        tileset->porytiles_component().middle(std::get<1>(demetatileized_images));
        tileset->porytiles_component().top(std::get<2>(demetatileized_images));

        std::ignore = repo.save(*tileset);
    }

  private:
    static constexpr auto kCommandName = "debug-normalize";
    static constexpr auto kCommandDesc =
        "Load a tileset, normalize it, and write the normalized tiles back to the layer images.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
};

class DebugPrimaryCompileCommand final : public Command {
  public:
    explicit DebugPrimaryCompileCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to compile")->required();
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
            return;
        }
        const auto tileset = std::move(maybe_tileset.value());

        // Compile the tileset
        auto compile_result = compiler.compile(*tileset);
        if (!compile_result.has_value()) {
            for (const auto &err : compile_result.chain()) {
                std::cerr << err->details(formatter) << std::endl;
            }
            return;
        }
        const auto new_tileset = std::move(compile_result.value());

        // Save the tileset back
        const auto new_tileset_save_result = repo.save(*new_tileset);
        if (!new_tileset_save_result.has_value()) {
            std::cerr << new_tileset_save_result.error() << std::endl;
            return;
        }
    }

  private:
    static constexpr auto kCommandName = "debug-compile-primary";
    static constexpr auto kCommandDesc =
        "Load a tileset, run it through the compile-primary service, and write it back.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
};
