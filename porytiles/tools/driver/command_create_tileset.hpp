#pragma once

#include <memory>
#include <string>
#include <unistd.h>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles/app/use_cases/create_primary_tileset.hpp"
#include "porytiles/app/use_cases/create_secondary_tileset.hpp"
#include "porytiles/domain/repos/tileset_repo.hpp"
#include "porytiles/domain/services/palette_printer.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/domain/services/tileset_compiler.hpp"
#include "porytiles/domain/services/tileset_creator.hpp"
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

class CreateTilesetCommand final : public Command {
  public:
    explicit CreateTilesetCommand(CLI::App &parent_app) : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to create (e.g., gTileset_MyTileset)")
            ->required();
        cmd.add_flag("--secondary", secondary_, "Create a secondary tileset instead of a primary.");
        project_root_opt_.RegisterOpt(cmd);
        porytiles::register_config_options(cmd, cli_storage_);
    }

    void Run() override
    {
        using namespace porytiles;

        // Use Fruit DI to inject TextFormatter based on no_color flag
        const bool no_color = !isatty(STDERR_FILENO);
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

        // Setup metadata provider and tileset manager
        ProjectTilesetMetadataProvider metadata_provider{project_root, text_formatter, diag.get()};
        ProjectLayoutMetadataProvider layout_metadata_provider{project_root, text_formatter, diag.get()};
        ProjectTilesetMetadataWriter metadata_writer{project_root, text_formatter};
        IncbinDeclarationAppender incbin_appender{project_root, text_formatter};
        ProjectTilesetAnimsModifier tileset_anims_modifier{project_root, &config, diag.get()};

        // Resolve the per-tileset attribute schema and build one enum provider per provider-backed field. The schema
        // and provider map must outlive the CSV loader, artifact writer, and creator below, which hold pointers into
        // them.
        MetatilesHeaderProvider metatiles_header{project_root, text_formatter};
        TilesetAttrSchemaResolver schema_resolver{
            &config, &layout_metadata_provider, &metatiles_header, text_formatter, diag.get()};
        auto resolved_result = schema_resolver.resolve(tileset_name_);
        if (!resolved_result.has_value()) {
            diag->fatal(resolved_result);
            throw CLI::RuntimeError{1};
        }
        const ResolvedTilesetAttrSchema resolved = std::move(resolved_result).value();
        ProviderMap provider_map = build_provider_map(project_root, resolved.schema, text_formatter, diag.get());

        // The tileset manager takes the resolved schema so its generated INCBIN declarations match the
        // binary attribute format, so it must be constructed after schema resolution.
        ProjectPorytilesTilesetManager tileset_manager{
            project_root,
            &metadata_provider,
            &metadata_writer,
            &config,
            &resolved.schema,
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
            &resolved.schema,
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

        // Setup creator and compiler. The creator seeds its sample art with a behavior constant name, so it needs the
        // behavior field's provider. A schema without a provider-backed behavior field cannot support creation, so fail
        // fast here rather than letting the creator resolve names against nothing.
        const auto behavior_provider_it = provider_map.find(attr::field_behavior);
        if (behavior_provider_it == provider_map.end()) {
            const auto no_behavior_err = ChainableResult<void>{FormattableError{
                "Cannot create a tileset: the resolved attribute schema has no provider-backed '{}' field.",
                FormatParam{std::string{attr::field_behavior}, Style::bold}}};
            diag->fatal(no_behavior_err);
            throw CLI::RuntimeError{1};
        }
        TilesetCreator creator{&config, behavior_provider_it->second.get()};
        TilesetCompiler compiler{
            &config, &resolved.schema, text_formatter, diag.get(), tile_printer.get(), pal_printer.get()};

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
    porytiles::CliOptionStorage cli_storage_;
};
