#pragma once

#include <memory>
#include <string>
#include <unistd.h>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles2/app/use_cases/create_primary_tileset.hpp"
#include "porytiles2/app/use_cases/create_secondary_tileset.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/domain/services/tileset_compiler.hpp"
#include "porytiles2/domain/services/tileset_creator.hpp"
#include "porytiles2/infra/cli/cli_option_registration.hpp"
#include "porytiles2/infra/cli/cli_option_storage.hpp"
#include "porytiles2/infra/config/cli_option_provider.hpp"
#include "porytiles2/infra/config/default_provider.hpp"
#include "porytiles2/infra/config/header_define_provider.hpp"
#include "porytiles2/infra/config/lazy_layered_config.hpp"
#include "porytiles2/infra/config/metatiles_header_provider.hpp"
#include "porytiles2/infra/config/yaml_file_provider.hpp"
#include "porytiles2/infra/repos/project_artifact_checksum_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_writer.hpp"
#include "porytiles2/infra/services/ascii_tile_printer.hpp"
#include "porytiles2/infra/services/attributes_csv_loader.hpp"
#include "porytiles2/infra/services/base_game_detector.hpp"
#include "porytiles2/infra/services/color_palette_printer.hpp"
#include "porytiles2/infra/services/header_behavior_map_provider.hpp"
#include "porytiles2/infra/services/header_encounter_type_map_provider.hpp"
#include "porytiles2/infra/services/header_terrain_type_map_provider.hpp"
#include "porytiles2/infra/services/incbin_declaration_appender.hpp"
#include "porytiles2/infra/services/jasc_pal_loader.hpp"
#include "porytiles2/infra/services/jasc_pal_saver.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"
#include "porytiles2/infra/services/project_layout_metadata_provider.hpp"
#include "porytiles2/infra/services/project_porytiles_tileset_manager.hpp"
#include "porytiles2/infra/services/project_tileset_anims_modifier.hpp"
#include "porytiles2/infra/services/project_tileset_metadata_writer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/di/components.hpp"
#include "porytiles2/xcut/diagnostics/diagnostic_tag_filter.hpp"
#include "porytiles2/xcut/diagnostics/filtered_user_diagnostics.hpp"
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
        cmd.add_flag("--secondary", secondary_, "Create a secondary tileset instead of a primary.");
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

        // Create unfiltered diag for config bootstrapping (so config-loading warnings always show)
        auto stderr_diag = std::make_unique<StderrStyledUserDiagnostics>(text_formatter);

        /*
         * TODO: below we're passing hardcoded "include/" for structural project files. At some point we'll want the
         * CLI tool to provide a way for users to change these values, in case:
         * - they moved fieldmap.h, metatile_behaviors.h, etc to a different location
         */
        std::filesystem::path project_root = project_root_opt_.project_root();
        std::filesystem::path fieldmap_header_root_relative{"include/fieldmap.h"};
        std::filesystem::path behaviors_header_root_relative{"include/constants/metatile_behaviors.h"};
        std::filesystem::path global_fieldmap_header_root_relative{"include/global.fieldmap.h"};

        // Setup layered configuration (CLI options have highest priority)
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<CliOptionProvider>(cli_storage_));
        auto yaml_provider = std::make_unique<YamlFileProvider>(text_formatter, stderr_diag.get(), project_root);
        auto *yaml_provider_ptr = yaml_provider.get();
        providers.push_back(std::move(yaml_provider));
        providers.push_back(
            std::make_unique<HeaderDefineProvider>(project_root, fieldmap_header_root_relative, text_formatter));
        providers.push_back(std::make_unique<MetatilesHeaderProvider>(project_root, text_formatter));
        providers.push_back(std::make_unique<DefaultProvider>());
        LazyLayeredConfig config{text_formatter, std::move(providers)};

        // Eagerly validate all YAML config files for unknown keys
        if (yaml_provider_ptr->preload_and_validate(ConfigScopeType::tileset, tileset_name_)) {
            const auto validation_err = ChainableResult<void>{FormattableError{
                "Configuration validation failed for tileset '{}'.", FormatParam{tileset_name_, Style::bold}}};
            stderr_diag->fatal(validation_err);
            throw CLI::RuntimeError{1};
        }

        // Helper to safely extract filter patterns from config, falling back to empty on error
        auto get_filter_patterns =
            [&](ChainableResult<ConfigValue<std::vector<std::string>>> result) -> std::vector<std::string> {
            if (result.has_value()) {
                return std::move(result).value().value();
            }
            stderr_diag->fatal(result);
            return {};
        };

        // Build diagnostic filters from config values
        DiagnosticTagFilter warning_filter{
            get_filter_patterns(config.diagnostic_warnings_exclude(ConfigScopeType::tileset, tileset_name_)),
            get_filter_patterns(config.diagnostic_warnings_include(ConfigScopeType::tileset, tileset_name_))};

        DiagnosticTagFilter remark_filter{
            get_filter_patterns(config.diagnostic_remarks_exclude(ConfigScopeType::tileset, tileset_name_)),
            get_filter_patterns(config.diagnostic_remarks_include(ConfigScopeType::tileset, tileset_name_))};

        // Wrap with filter decorator for all subsequent operations
        auto diag = std::make_unique<FilteredUserDiagnostics>(
            text_formatter, stderr_diag.get(), std::move(warning_filter), std::move(remark_filter));

        std::unique_ptr<TilePrinter> tile_printer = std::make_unique<AsciiTilePrinter>(text_formatter);
        std::unique_ptr<PalettePrinter> pal_printer = std::make_unique<ColorPalettePrinter>(text_formatter);

        // Initialize stateless services
        PngRgbaImageLoader png_rgba_loader{};
        PngIndexedImageLoader png_indexed_loader{};
        PngRgbaImageSaver png_rgba_saver{};
        PngIndexedImageSaver png_indexed_saver{};
        JascPalLoader jasc_loader{text_formatter};
        JascPalSaver jasc_saver{text_formatter};
        AnimJsonParser anim_json_parser{text_formatter};
        AnimCodeParser anim_code_parser{text_formatter, diag.get()};
        AnimCodeGenerator anim_code_generator{};

        // Setup behavior map provider
        HeaderBehaviorMapProvider behavior_map_provider{
            project_root / behaviors_header_root_relative, text_formatter, diag.get()};

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

        // Detect base game
        BaseGameDetector base_game_detector{project_root, text_formatter, diag.get()};
        auto base_game_result = base_game_detector.detect();
        if (!base_game_result.has_value()) {
            diag->fatal(base_game_result);
            throw CLI::RuntimeError{1};
        }
        const BaseGame base_game = base_game_result.value();

        // Conditionally create terrain/encounter providers for FireRed
        std::unique_ptr<HeaderTerrainTypeMapProvider> terrain_provider;
        std::unique_ptr<HeaderEncounterTypeMapProvider> encounter_provider;
        if (base_game == BaseGame::pokefirered) {
            terrain_provider = std::make_unique<HeaderTerrainTypeMapProvider>(
                project_root / global_fieldmap_header_root_relative, text_formatter, diag.get());
            encounter_provider = std::make_unique<HeaderEncounterTypeMapProvider>(
                project_root / global_fieldmap_header_root_relative, text_formatter, diag.get());
        }

        // Setup attributes CSV loader (after base game detection for format validation)
        AttributesCsvLoader attributes_csv_loader{
            text_formatter, &behavior_map_provider, base_game, terrain_provider.get(), encounter_provider.get()};

        // Setup the tileset repository
        ProjectTilesetArtifactKeyProvider key_provider{project_root, &config, text_formatter, diag.get()};
        ProjectTilesetArtifactReader artifact_reader{
            project_root,
            base_game,
            &png_rgba_loader,
            &png_indexed_loader,
            &jasc_loader,
            &attributes_csv_loader,
            &anim_json_parser,
            &anim_code_parser,
            &metadata_provider};
        ProjectTilesetArtifactWriter artifact_writer{
            &config,
            &config,
            project_root,
            base_game,
            text_formatter,
            diag.get(),
            &png_rgba_saver,
            &png_indexed_saver,
            &jasc_saver,
            &anim_json_parser,
            &anim_code_generator,
            &behavior_map_provider,
            terrain_provider.get(),
            encounter_provider.get()};
        ProjectArtifactChecksumProvider checksum_provider{project_root};
        TilesetRepo repo{
            &checksum_provider, &metadata_provider, &key_provider, &artifact_reader, &artifact_writer, diag.get()};

        // Setup creator and compiler
        TilesetCreator creator{&config, &behavior_map_provider};
        TilesetCompiler compiler{&config, text_formatter, diag.get(), tile_printer.get(), pal_printer.get()};

        // Setup layout metadata provider (needed for secondary tileset primary pairing)
        ProjectLayoutMetadataProvider layout_metadata_provider{project_root, text_formatter, diag.get()};

        // Create and run the appropriate use case based on --secondary flag
        ChainableResult<void> create_result;
        if (secondary_) {
            CreateSecondaryTileset create_use_case{
                &creator,
                &compiler,
                &repo,
                &metadata_provider,
                &layout_metadata_provider,
                &tileset_manager,
                &config,
                &config,
                diag.get()};
            create_result = create_use_case.create(tileset_name_);
        }
        else {
            CreatePrimaryTileset create_use_case{
                &creator, &compiler, &repo, &metadata_provider, &tileset_manager, &config, &config, diag.get()};
            create_result = create_use_case.create(tileset_name_);
        }
        if (!create_result.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to create tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                create_result};
            diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
    }

  private:
    static constexpr auto kCommandName = "create-tileset";
    static constexpr auto kCommandDesc = "Create a new Porytiles-managed tileset from scratch.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
    bool secondary_ = false;
    OptProjectRoot project_root_opt_;
    porytiles2::CliOptionStorage cli_storage_;
};
