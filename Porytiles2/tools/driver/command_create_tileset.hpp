#pragma once

#include <memory>
#include <string>
#include <unistd.h>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles2/app/use_cases/create_primary_tileset.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/domain/services/primary_tileset_creator.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/infra/cli/cli_option_registration.hpp"
#include "porytiles2/infra/cli/cli_option_storage.hpp"
#include "porytiles2/infra/config/cli_option_provider.hpp"
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
#include "porytiles2/infra/services/incbin_declaration_appender.hpp"
#include "porytiles2/infra/services/jasc_pal_loader.hpp"
#include "porytiles2/infra/services/jasc_pal_saver.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"
#include "porytiles2/infra/services/project_porytiles_tileset_manager.hpp"
#include "porytiles2/infra/services/project_tileset_anims_modifier.hpp"
#include "porytiles2/infra/services/project_tileset_metadata_writer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/di/components.hpp"
#include "porytiles2/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

#include "command.hpp"
#include "option.hpp"

class CreateTilesetCommand final : public Command {
  public:
    explicit CreateTilesetCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to create (e.g., gTileset_MyTileset)")
            ->required();
        project_root_opt_.RegisterOpt(cmd);
        porytiles2::register_config_options(cmd, cli_storage_);
    }

    void Run() override
    {
        using namespace porytiles2;

        // Use Fruit DI to inject TextFormatter based on no_color flag
        const bool no_color = !isatty(STDERR_FILENO);
        fruit::Injector injector{di::get_formatter_component, no_color};
        auto text_formatter = injector.get<TextFormatter *>();

        // Manually create other services (not yet using DI for these)
        std::unique_ptr<UserDiagnostics> diag = std::make_unique<StderrStyledUserDiagnostics>(text_formatter);

        /*
         * TODO: below we're passing hardcoded "include/" for structural project files. At some point we'll want the
         * CLI tool to provide a way for users to change these values, in case:
         * - they moved fieldmap.h, metatile_behaviors.h, etc to a different location
         */
        std::filesystem::path project_root = project_root_opt_.project_root();
        std::filesystem::path fieldmap_header_root_relative{"include/fieldmap.h"};
        std::filesystem::path behaviors_header_root_relative{"include/constants/metatile_behaviors.h"};

        // Setup layered configuration (CLI options have highest priority)
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<CliOptionProvider>(cli_storage_));
        providers.push_back(std::make_unique<YamlFileProvider>(text_formatter, diag.get(), project_root));
        providers.push_back(
            std::make_unique<HeaderDefineProvider>(project_root, fieldmap_header_root_relative, text_formatter));
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
        AnimYamlParser anim_yaml_parser{text_formatter};
        AnimCodeParser anim_code_parser{text_formatter, diag.get()};
        AnimCodeGenerator anim_code_generator{};

        // Setup behavior map provider and attributes CSV loader
        HeaderBehaviorMapProvider behavior_map_provider{
            project_root / behaviors_header_root_relative, text_formatter, diag.get()};
        AttributesCsvLoader attributes_csv_loader{text_formatter, &behavior_map_provider};

        // Setup metadata provider and tileset manager
        ProjectTilesetMetadataProvider metadata_provider{project_root, text_formatter, diag.get()};
        ProjectTilesetMetadataWriter metadata_writer{project_root, text_formatter};
        IncbinDeclarationAppender incbin_appender{project_root, text_formatter};
        ProjectTilesetAnimsModifier tileset_anims_modifier{project_root, &config, diag.get()};
        ProjectPorytilesTilesetManager tileset_manager{
            project_root,
            &metadata_provider,
            &metadata_writer,
            &config,
            diag.get(),
            &incbin_appender,
            &tileset_anims_modifier};

        // Setup the tileset repository
        ProjectTilesetArtifactKeyProvider key_provider{project_root, &config, text_formatter, diag.get()};
        ProjectTilesetArtifactReader artifact_reader{
            project_root,
            &png_rgba_loader,
            &png_indexed_loader,
            &jasc_loader,
            &attributes_csv_loader,
            &anim_yaml_parser,
            &anim_code_parser,
            &metadata_provider};
        ProjectTilesetArtifactWriter artifact_writer{
            &config,
            &config,
            project_root,
            text_formatter,
            diag.get(),
            &png_rgba_saver,
            &png_indexed_saver,
            &jasc_saver,
            &anim_yaml_parser,
            &anim_code_generator,
            &behavior_map_provider};
        ProjectArtifactChecksumProvider checksum_provider{project_root};
        TilesetRepo repo{
            &checksum_provider, &metadata_provider, &key_provider, &artifact_reader, &artifact_writer, diag.get()};

        // Setup creator and compiler
        PrimaryTilesetCreator creator{&config, &behavior_map_provider};
        PrimaryTilesetCompiler compiler{&config, text_formatter, diag.get(), tile_printer.get(), pal_printer.get()};

        // Create the use case
        CreatePrimaryTileset create_use_case{
            &creator, &compiler, &repo, &metadata_provider, &tileset_manager, &config, &config, diag.get()};

        // Run the use case
        auto create_result = create_use_case.create(tileset_name_);
        if (!create_result.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to create tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                create_result};
            diag->fatal(fail_result);
        }
    }

  private:
    static constexpr auto kCommandName = "create-tileset";
    static constexpr auto kCommandDesc = "Create a new Porytiles-managed tileset from scratch.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
    OptProjectRoot project_root_opt_;
    porytiles2::CliOptionStorage cli_storage_;
};
