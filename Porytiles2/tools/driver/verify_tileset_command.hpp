#pragma once

#include "CLI/CLI.hpp"

#include "porytiles2/app/use_cases/verify_primary_tileset.hpp"

#include "command.hpp"
#include "option_group.hpp"

class VerifyTilesetCommand final : public Command {
  public:
    explicit VerifyTilesetCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to load")->required();
        diagnostics_opts_.RegisterGroup(cmd);
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

        VerifyPrimaryTileset verify_use_case{&repo};
        auto verify_result = verify_use_case.verify(tileset_name_);
        if (!verify_result.has_value()) {
            for (const auto &err : verify_result.chain()) {
                std::cerr << err->details(formatter) << std::endl;
            }
        }
    }

  private:
    static constexpr auto kCommandName = "verify-tileset";
    static constexpr auto kCommandDesc = "Verify a tileset's contents.";
    static constexpr auto kCommandGroup = "COMMANDS";

    std::string tileset_name_;
    OptGroupDiagnostics diagnostics_opts_;
};
