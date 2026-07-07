#pragma once

#include <memory>
#include <string>
#include <unistd.h>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles/app/use_cases/decompile_primary_tileset.hpp"
#include "porytiles/domain/repos/tileset_repo.hpp"
#include "porytiles/domain/services/palette_printer.hpp"
#include "porytiles/domain/services/primary_tileset_decompiler.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/domain/services/tileset_compiler.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/infra/config/cli_option_provider.hpp"
#include "porytiles/infra/config/default_provider.hpp"
#include "porytiles/infra/config/header_define_provider.hpp"
#include "porytiles/infra/config/lazy_layered_config.hpp"
#include "porytiles/infra/config/metatile_attribute_config_provider.hpp"
#include "porytiles/infra/config/metatiles_header_provider.hpp"
#include "porytiles/infra/config/yaml_file_provider.hpp"
#include "porytiles/infra/repos/project_artifact_checksum_provider.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_reader.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_writer.hpp"
#include "porytiles/infra/services/ascii_tile_printer.hpp"
#include "porytiles/infra/services/attributes_csv_loader.hpp"
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
#include "porytiles/infra/services/tileset_attr_schema_resolver.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/di/components.hpp"
#include "porytiles/xcut/diagnostics/diagnostic_tag_filter.hpp"
#include "porytiles/xcut/diagnostics/filtered_user_diagnostics.hpp"
#include "porytiles/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

#include "command.hpp"
#include "option.hpp"

class DecompileTilesetCommand final : public Command {
  public:
    explicit DecompileTilesetCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to decompile")->required();
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

        // Setup layered configuration (CLI options have highest priority)
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<CliOptionProvider>(cli_storage_));
        auto yaml_provider = std::make_unique<YamlFileProvider>(text_formatter, stderr_diag.get(), project_root);
        auto *yaml_provider_ptr = yaml_provider.get();
        providers.push_back(std::move(yaml_provider));
        providers.push_back(
            std::make_unique<HeaderDefineProvider>(project_root, fieldmap_header_root_relative, text_formatter));
        providers.push_back(

            std::make_unique<MetatileAttributeConfigProvider>(project_root, text_formatter, stderr_diag.get()));
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

        // Eagerly validate metatile-attr-size to fail fast before any file I/O. The effective attribute width used
        // below is the schema resolver's attr_bytes, which starts from this config value but may widen it.
        auto attr_size_check = config.metatile_attr_size(ConfigScopeType::tileset, tileset_name_);
        if (!attr_size_check.has_value()) {
            stderr_diag->fatal(attr_size_check);
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

        // Setup primary importer and compiler
        PrimaryTilesetDecompiler decompiler{&config, text_formatter, diag.get(), tile_printer.get(), pal_printer.get()};
        TilesetCompiler compiler{&config, text_formatter, diag.get(), tile_printer.get(), pal_printer.get()};

        // Setup metadata provider, tileset manager
        ProjectTilesetMetadataProvider metadata_provider{project_root, text_formatter, diag.get()};
        ProjectLayoutMetadataProvider layout_metadata_provider{project_root, text_formatter, diag.get()};
        ProjectTilesetMetadataWriter metadata_writer{project_root, text_formatter};
        IncbinDeclarationAppender incbin_appender{project_root, text_formatter};
        ProjectTilesetAnimsModifier tileset_anims_modifier{project_root, &config, diag.get()};

        // Resolve the per-tileset attribute schema and build one enum provider per provider-backed field. The schema
        // and provider map must outlive the CSV loader and artifact writer below, which hold pointers into them.
        TilesetAttrSchemaResolver schema_resolver{&config, &layout_metadata_provider, text_formatter, diag.get()};
        auto resolved_result = schema_resolver.resolve(tileset_name_);
        if (!resolved_result.has_value()) {
            diag->fatal(resolved_result);
            throw CLI::RuntimeError{1};
        }
        const ResolvedTilesetAttrSchema resolved = std::move(resolved_result).value();
        ProviderMap provider_map = build_provider_map(project_root, resolved.schema, text_formatter, diag.get());

        // The tileset manager takes the resolved attribute width so its generated INCBIN declarations match the
        // binary attribute format, so it must be constructed after schema resolution.
        ProjectPorytilesTilesetManager tileset_manager{
            project_root,
            &metadata_provider,
            &metadata_writer,
            &config,
            resolved.attr_bytes,
            diag.get(),
            &incbin_appender,
            &tileset_anims_modifier};

        // Setup attributes CSV loader
        AttributesCsvLoader attributes_csv_loader{text_formatter, &resolved.schema, &provider_map, &config, diag.get()};

        // Setup the tileset repository
        ProjectTilesetArtifactKeyProvider key_provider{
            project_root, &config, &metadata_provider, text_formatter, diag.get()};
        ProjectTilesetArtifactReader artifact_reader{
            project_root,
            resolved.attr_bytes,
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
            &resolved.schema,
            &provider_map,
            resolved.attr_bytes,
            text_formatter,
            diag.get(),
            &png_rgba_saver,
            &png_indexed_saver,
            &jasc_saver,
            &anim_json_parser,
            &anim_code_generator};
        ProjectArtifactChecksumProvider checksum_provider{project_root};
        TilesetRepo repo{
            &checksum_provider, &metadata_provider, &key_provider, &artifact_reader, &artifact_writer, diag.get()};

        DecompilePrimaryTileset decompile_use_case{
            &repo, &decompiler, &compiler, &metadata_provider, &tileset_manager, &config, &config, diag.get()};

        // Run the use case
        auto decompile_result = decompile_use_case.decompile(tileset_name_);
        if (!decompile_result.has_value()) {
            const auto fail_result = ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to decompile tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                decompile_result};
            diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
    }

  private:
    static constexpr auto kCommandName = "decompile-tileset";
    static constexpr auto kCommandDesc =
        "Decompile a tileset -- update the Porytiles assets to match the Porymap assets.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
