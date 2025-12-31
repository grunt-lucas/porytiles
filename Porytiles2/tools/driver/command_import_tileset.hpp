#pragma once

#include <memory>
#include <string>
#include <unistd.h>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles2/app/use_cases/import_primary_tileset.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/domain/services/primary_tileset_importer.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/infra/config/default_provider.hpp"
#include "porytiles2/infra/config/header_define_provider.hpp"
#include "porytiles2/infra/config/lazy_layered_config.hpp"
#include "porytiles2/infra/config/yaml_file_provider.hpp"
#include "porytiles2/infra/repos/project_artifact_checksum_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_writer.hpp"
#include "porytiles2/infra/services/ascii_tile_printer.hpp"
#include "porytiles2/infra/services/attributes_csv_loader.hpp"
#include "porytiles2/infra/services/color_palette_printer.hpp"
#include "porytiles2/infra/services/header_behavior_map_provider.hpp"
#include "porytiles2/infra/services/jasc_pal_loader.hpp"
#include "porytiles2/infra/services/jasc_pal_saver.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/di/components.hpp"
#include "porytiles2/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

#include "command.hpp"

class ImportTilesetCommand final : public Command {
  public:
    explicit ImportTilesetCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to import")->required();
    }

    void Run() override
    {
        using namespace porytiles2;

        /*
         * TODO: once we have more compilation code finished, we should come back and do more dependency injection via
         * Fruit.
         */
        // Use Fruit DI to inject TextFormatter based on no_color flag
        const bool no_color = !isatty(STDERR_FILENO); // Disable color when stderr is not a terminal
        fruit::Injector injector{di::get_formatter_component, no_color};
        auto text_formatter = injector.get<TextFormatter *>();

        // Manually create other services (not yet using DI for these)
        std::unique_ptr<UserDiagnostics> diag = std::make_unique<StderrStyledUserDiagnostics>(text_formatter);

        // Setup layered configuration
        ProjectTilesetArtifactKeyProvider key_provider{".", text_formatter, diag.get()};
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<YamlFileProvider>(text_formatter, diag.get(), ".", key_provider));
        providers.push_back(std::make_unique<HeaderDefineProvider>(".", "include/fieldmap.h", text_formatter));
        providers.push_back(std::make_unique<DefaultProvider>());
        LazyLayeredConfig config{text_formatter, std::move(providers)};

        std::unique_ptr<TilePrinter> tile_printer = std::make_unique<AsciiTilePrinter>(text_formatter);
        std::unique_ptr<PalettePrinter> pal_printer = std::make_unique<ColorPalettePrinter>(text_formatter);

        // Initialize stateless services
        PngRgbaImageLoader png_rgba_loader{};
        PngIndexedImageLoader png_indexed_loader{};
        PngRgbaImageSaver png_rgba_saver{};
        PngIndexedImageSaver png_indexed_saver{};
        JascPalLoader jasc_loader{text_formatter};
        JascPalSaver jasc_saver{text_formatter};
        AnimYamlParser anim_yaml_parser{};
        AnimCodeParser anim_code_parser{text_formatter, diag.get()};
        AnimCodeGenerator anim_code_generator{};

        // Setup primary importer and compiler
        PrimaryTilesetImporter importer{&config, text_formatter, diag.get(), tile_printer.get(), pal_printer.get()};
        PrimaryTilesetCompiler compiler{&config, text_formatter, diag.get(), tile_printer.get(), pal_printer.get()};

        // Setup behavior map provider and attributes CSV loader
        HeaderBehaviorMapProvider behavior_map_provider{
            ".", "include/constants/metatile_behaviors.h", text_formatter, diag.get()};
        AttributesCsvLoader attributes_csv_loader{text_formatter, &behavior_map_provider};

        // Setup the tileset repository
        ProjectTilesetArtifactReader artifact_reader{
            &png_rgba_loader,
            &png_indexed_loader,
            &jasc_loader,
            &attributes_csv_loader,
            &anim_yaml_parser,
            &anim_code_parser};
        ProjectTilesetArtifactWriter artifact_writer{
            &config, ".", &png_rgba_saver, &png_indexed_saver, &jasc_saver, &anim_yaml_parser, &anim_code_generator};
        // We already set this up earlier for the Yaml config provider
        // ProjectTilesetArtifactKeyProvider key_provider{"."};
        ProjectArtifactChecksumProvider checksum_provider{&key_provider};
        TilesetRepo repo{
            &checksum_provider, &key_provider, &artifact_reader, &artifact_writer, text_formatter, diag.get()};

        ImportPrimaryTileset import_use_case{&repo, &importer, &compiler, &config, &config, text_formatter, diag.get()};

        // Run the use case
        auto import_result = import_use_case.import(tileset_name_);
        if (!import_result.has_value()) {
            const auto fail_result = ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to import tileset '{}'", FormatParam{tileset_name_, Style::bold}},
                import_result};
            diag->fatal(fail_result);
        }
    }

  private:
    static constexpr auto kCommandName = "import-tileset";
    static constexpr auto kCommandDesc =
        "Import a tileset, i.e., update the Porytiles assets to match the Porymap assets.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
};
