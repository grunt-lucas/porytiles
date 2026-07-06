#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unistd.h>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles/app/use_cases/compile_primary_tileset.hpp"
#include "porytiles/app/use_cases/compile_secondary_tileset.hpp"
#include "porytiles/domain/repos/tileset_repo.hpp"
#include "porytiles/domain/services/layer_image_metatileizer.hpp"
#include "porytiles/domain/services/palette_printer.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/domain/services/tileset_compiler.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/infra/config/cli_option_provider.hpp"
#include "porytiles/infra/config/default_provider.hpp"
#include "porytiles/infra/config/header_define_provider.hpp"
#include "porytiles/infra/config/lazy_layered_config.hpp"
#include "porytiles/infra/config/metatiles_header_provider.hpp"
#include "porytiles/infra/config/yaml_file_provider.hpp"
#include "porytiles/infra/repos/project_artifact_checksum_provider.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_reader.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_writer.hpp"
#include "porytiles/infra/services/ascii_tile_printer.hpp"
#include "porytiles/infra/services/attributes_csv_loader.hpp"
#include "porytiles/infra/services/base_game_detector.hpp"
#include "porytiles/infra/services/color_palette_printer.hpp"
#include "porytiles/infra/services/header_enum_map_provider.hpp"
#include "porytiles/infra/services/incbin_declaration_appender.hpp"
#include "porytiles/infra/services/jasc_pal_loader.hpp"
#include "porytiles/infra/services/jasc_pal_saver.hpp"
#include "porytiles/infra/services/png_indexed_image_loader.hpp"
#include "porytiles/infra/services/png_indexed_image_saver.hpp"
#include "porytiles/infra/services/png_rgba_image_loader.hpp"
#include "porytiles/infra/services/png_rgba_image_saver.hpp"
#include "porytiles/infra/services/project_layout_metadata_provider.hpp"
#include "porytiles/infra/services/project_porytiles_tileset_manager.hpp"
#include "porytiles/infra/services/project_tileset_anims_modifier.hpp"
#include "porytiles/infra/services/project_tileset_metadata_writer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/di/components.hpp"
#include "porytiles/xcut/diagnostics/diagnostic_tag_filter.hpp"
#include "porytiles/xcut/diagnostics/filtered_user_diagnostics.hpp"
#include "porytiles/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

#include "command.hpp"
#include "interim_enum_specs.hpp"
#include "option.hpp"

class CompileTilesetCommand final : public Command {
  public:
    explicit CompileTilesetCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to compile")->required();
        project_root_opt_.RegisterOpt(cmd);
        porytiles::register_config_options(cmd, cli_storage_);
    }

    void Run() override
    {
        using namespace porytiles;

        // Use Fruit DI to inject TextFormatter based on no_color flag
        const bool no_color = !isatty(STDERR_FILENO); // Disable color when stderr is not a terminal
        fruit::Injector injector{di::get_formatter_component, no_color};
        auto text_formatter = injector.get<TextFormatter *>();

        // Create unfiltered diag for config bootstrapping (so config-loading warnings always show)
        auto stderr_diag = std::make_unique<StderrStyledUserDiagnostics>(text_formatter);

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

        // Eagerly validate metatile-attr-size to fail fast before any file I/O
        auto attr_size_check = config.metatile_attr_size(ConfigScopeType::tileset, tileset_name_);
        if (!attr_size_check.has_value()) {
            stderr_diag->fatal(attr_size_check);
            throw CLI::RuntimeError{1};
        }
        const std::size_t metatile_attr_size = attr_size_check.value().value();

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

        // Setup primary compiler
        TilesetCompiler compiler{&config, text_formatter, diag.get(), tile_printer.get(), pal_printer.get()};

        // Setup behavior map provider
        HeaderEnumMapProvider behavior_map_provider{
            project_root / behaviors_header_root_relative, behavior_enum_spec(), text_formatter, diag.get()};

        // Setup metadata provider (needed by artifact reader for animation param loading)
        ProjectTilesetMetadataProvider metadata_provider{project_root, text_formatter, diag.get()};
        ProjectLayoutMetadataProvider layout_metadata_provider{project_root, text_formatter, diag.get()};

        // Setup Porytiles tileset manager and its dependencies
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
        std::unique_ptr<HeaderEnumMapProvider> terrain_provider;
        std::unique_ptr<HeaderEnumMapProvider> encounter_provider;
        if (base_game == BaseGame::pokefirered) {
            terrain_provider = std::make_unique<HeaderEnumMapProvider>(
                project_root / global_fieldmap_header_root_relative, terrain_enum_spec(), text_formatter, diag.get());
            encounter_provider = std::make_unique<HeaderEnumMapProvider>(
                project_root / global_fieldmap_header_root_relative, encounter_enum_spec(), text_formatter, diag.get());
        }

        // Setup attributes CSV loader (after base game detection for format validation)
        AttributesCsvLoader attributes_csv_loader{
            text_formatter, &behavior_map_provider, base_game, terrain_provider.get(), encounter_provider.get()};

        // Setup the tileset repository
        ProjectTilesetArtifactKeyProvider key_provider{
            project_root, &config, &metadata_provider, text_formatter, diag.get()};
        ProjectTilesetArtifactReader artifact_reader{
            project_root,
            metatile_attr_size,
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
            metatile_attr_size,
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

        // Verify the tileset exists in the project before proceeding
        if (!metadata_provider.exists(tileset_name_)) {
            const auto not_found_err = ChainableResult<void>{FormattableError{
                "Tileset '{}' does not exist. Create or import it first.", FormatParam{tileset_name_, Style::bold}}};
            diag->fatal(not_found_err);
            throw CLI::RuntimeError{1};
        }

        // Detect primary vs secondary and dispatch to the correct use case
        auto is_secondary_result = metadata_provider.is_secondary(tileset_name_);
        if (!is_secondary_result.has_value()) {
            diag->fatal(is_secondary_result);
            throw CLI::RuntimeError{1};
        }

        ChainableResult<void> compile_result;
        if (is_secondary_result.value()) {
            CompileSecondaryTileset compile_use_case{
                &repo,
                &compiler,
                &metadata_provider,
                &layout_metadata_provider,
                &tileset_manager,
                &config,
                &config,
                diag.get()};
            compile_result = compile_use_case.compile(tileset_name_);
        }
        else {
            CompilePrimaryTileset compile_use_case{
                &repo, &compiler, &metadata_provider, &tileset_manager, &config, &config, diag.get()};
            compile_result = compile_use_case.compile(tileset_name_);
        }
        if (!compile_result.has_value()) {
            const auto fail_result = ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to compile tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                compile_result};
            diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
    }

  private:
    static constexpr auto kCommandName = "compile-tileset";
    static constexpr auto kCommandDesc =
        "Compile a tileset -- update the Porymap assets to match the Porytiles assets.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
