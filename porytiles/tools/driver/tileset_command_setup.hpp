#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"
#include "gsl/pointers"

#include "porytiles/domain/repos/tileset_repo.hpp"
#include "porytiles/domain/services/palette_printer.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/domain/services/tileset_compiler.hpp"
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
#include "porytiles/infra/services/anim_code_generator.hpp"
#include "porytiles/infra/services/anim_code_parser.hpp"
#include "porytiles/infra/services/anim_json_parser.hpp"
#include "porytiles/infra/services/ascii_tile_printer.hpp"
#include "porytiles/infra/services/attributes_csv_loader.hpp"
#include "porytiles/infra/services/color_palette_printer.hpp"
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
#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles/infra/services/project_tileset_metadata_writer.hpp"
#include "porytiles/infra/services/tileset_attr_schema_cache.hpp"
#include "porytiles/infra/services/tileset_attr_schema_resolver.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/terminal_width.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"
#include "porytiles/xcut/di/components.hpp"
#include "porytiles/xcut/diagnostics/diagnostic_tag_filter.hpp"
#include "porytiles/xcut/diagnostics/filtered_user_diagnostics.hpp"
#include "porytiles/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Bootstrap class so every tileset command can share common config and diagnostic setup.
///
/// @details
/// Owns the formatter injector, the unfiltered stderr diagnostics used during config loading, the layered config,
/// and the filtered diagnostics built from the config's diagnostic include/exclude patterns. Construction eagerly
/// validates the YAML config files for the tileset scope and fails the command on validation errors.
///
/// Members hold pointers into earlier members, so the class is not copyable and not movable. Commands should construct
/// it once on the stack.
class TilesetCommandEnv {
  private:
    /// @brief The layered config's provider list bundled with a typed handle to the YamlFileProvider inside it.
    ///
    /// @details
    /// make_provider_chain returns both pieces together so the constructor can hand ownership of the list to config
    /// while keeping the typed handle it needs for YAML validation. The handle points at the provider owned by the
    /// list (and, after construction, by config), so it stays valid for the env's whole lifetime.
    struct ProviderChain {
        std::vector<std::unique_ptr<ConfigProvider>> providers;
        gsl::not_null<YamlFileProvider *> yaml_provider;
    };

    [[nodiscard]] static ProviderChain make_provider_chain(
        const std::filesystem::path &project_root,
        const CliOptionStorage &cli_storage,
        TextFormatter *text_formatter,
        StderrStyledUserDiagnostics *stderr_diag)
    {
        // Layered configuration: CLI options have highest priority.
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<CliOptionProvider>(cli_storage));
        providers.push_back(std::make_unique<YamlFileProvider>(text_formatter, stderr_diag, project_root));
        // Take the handle from the owning slot so its provenance is the list itself, not a moved-from local.
        auto *yaml_provider = static_cast<YamlFileProvider *>(providers.back().get());
        providers.push_back(
            std::make_unique<HeaderDefineProvider>(
                project_root, std::filesystem::path{"include/fieldmap.h"}, text_formatter));
        providers.push_back(
            std::make_unique<MetatileAttributeConfigProvider>(project_root, text_formatter, stderr_diag));
        providers.push_back(std::make_unique<DefaultProvider>());
        return {std::move(providers), yaml_provider};
    }

  public:
    TilesetCommandEnv(std::filesystem::path root, const std::string &tileset_name, const CliOptionStorage &cli_storage)
        : project_root{std::move(root)}, injector{di::get_formatter_component, !isatty(STDERR_FILENO)},
          text_formatter{injector.get<TextFormatter *>()},
          stderr_diag{text_formatter, resolve_terminal_width(STDERR_FILENO)},
          provider_chain{make_provider_chain(project_root, cli_storage, text_formatter, &stderr_diag)},
          yaml_provider{provider_chain.yaml_provider}, config{text_formatter, std::move(provider_chain.providers)}
    {
        // Eagerly validate all YAML config files for unknown keys
        if (yaml_provider->preload_and_validate(ConfigScopeType::tileset, tileset_name)) {
            const auto validation_err = ChainableResult<void>{FormattableError{
                "Configuration validation failed for tileset '{}'.", FormatParam{tileset_name, Style::bold}}};
            stderr_diag.fatal(validation_err);
            throw CLI::RuntimeError{1};
        }

        // Helper to safely extract filter patterns from config, falling back to empty on error
        auto get_filter_patterns =
            [&](ChainableResult<ConfigValue<std::vector<std::string>>> result) -> std::vector<std::string> {
            if (result.has_value()) {
                return std::move(result).value().value();
            }
            stderr_diag.fatal(result);
            return {};
        };

        // Build diagnostic filters from config values
        DiagnosticTagFilter warning_filter{
            get_filter_patterns(config.diagnostic_warnings_exclude(ConfigScopeType::tileset, tileset_name)),
            get_filter_patterns(config.diagnostic_warnings_include(ConfigScopeType::tileset, tileset_name))};

        DiagnosticTagFilter remark_filter{
            get_filter_patterns(config.diagnostic_remarks_exclude(ConfigScopeType::tileset, tileset_name)),
            get_filter_patterns(config.diagnostic_remarks_include(ConfigScopeType::tileset, tileset_name))};

        // Wrap with filter decorator for all subsequent operations
        diag = std::make_unique<FilteredUserDiagnostics>(
            text_formatter, &stderr_diag, std::move(warning_filter), std::move(remark_filter));
    }

    TilesetCommandEnv(const TilesetCommandEnv &) = delete;
    TilesetCommandEnv &operator=(const TilesetCommandEnv &) = delete;
    TilesetCommandEnv(TilesetCommandEnv &&) = delete;
    TilesetCommandEnv &operator=(TilesetCommandEnv &&) = delete;

    std::filesystem::path project_root;
    fruit::Injector<TextFormatter> injector;
    TextFormatter *text_formatter;
    StderrStyledUserDiagnostics stderr_diag;

  private:
    // Bridges make_provider_chain's single return value across two member initializers: yaml_provider copies the
    // handle, config takes ownership of the vector. After construction the providers vector is moved-from and empty.
    ProviderChain provider_chain;

  public:
    // Typed handle to the YamlFileProvider owned by config's provider list, needed for eager YAML validation.
    gsl::not_null<YamlFileProvider *> yaml_provider;
    LazyLayeredConfig config;
    std::unique_ptr<FilteredUserDiagnostics> diag;
};

/// @brief The schema-driven service graph shared by the compile, create, import, and decompile commands.
///
/// @details
/// Builds the per-tileset schema cache (each tileset a command touches resolves its own schema and providers, so a
/// paired primary's artifacts decode with the primary's schema, not the target's) and wires the services that consume
/// it: the attributes CSV loader, the artifact reader/writer, the tileset repo, the tileset manager, and the compiler.
/// This is the single home for that wiring so the commands cannot drift apart.
///
/// Declaration order is dependency order: the schema cache and the target's entry come before the artifact
/// reader/writer, manager, and compiler, which hold pointers into them. That makes the graph self-pinning: not
/// copyable, not movable, constructed once on the stack after the env.
///
/// Construction fails the command (fatal diagnostic plus CLI::RuntimeError) when the target's schema resolution fails.
/// Schemas for other tilesets (a secondary's paired primary) resolve lazily on first artifact read, and a failure
/// there surfaces as that read's error.
class TilesetCommandServices {
  public:
    TilesetCommandServices(TilesetCommandEnv &env, const std::string &tileset_name)
        : tile_printer{std::make_unique<AsciiTilePrinter>(env.text_formatter)},
          pal_printer{std::make_unique<ColorPalettePrinter>(env.text_formatter)}, jasc_loader{env.text_formatter},
          jasc_saver{env.text_formatter}, anim_json_parser{env.text_formatter},
          anim_code_parser{env.text_formatter, env.diag.get()},
          metadata_provider{env.project_root, env.text_formatter, env.diag.get()},
          layout_metadata_provider{env.project_root, env.text_formatter, env.diag.get()},
          metadata_writer{env.project_root, env.text_formatter}, incbin_appender{env.project_root, env.text_formatter},
          tileset_anims_modifier{env.project_root, &env.config, env.diag.get()},
          metatiles_header{env.project_root, env.text_formatter},
          schema_resolver{
              &env.config, &layout_metadata_provider, &metatiles_header, env.text_formatter, env.diag.get()},
          schema_cache{env.project_root, &schema_resolver, env.text_formatter, env.diag.get()},
          target_entry{entry_or_fail(schema_cache, tileset_name, *env.diag)}, resolved{target_entry->resolved},
          provider_map{target_entry->providers},
          tileset_manager{
              env.project_root,
              &metadata_provider,
              &metadata_writer,
              &env.config,
              resolved.declaration_bytes,
              env.diag.get(),
              &incbin_appender,
              &tileset_anims_modifier},
          attributes_csv_loader{env.text_formatter, &env.config, env.diag.get()},
          key_provider{env.project_root, &env.config, &metadata_provider, env.text_formatter, env.diag.get()},
          artifact_reader{
              env.project_root,
              &schema_cache,
              &png_rgba_loader,
              &png_indexed_loader,
              &jasc_loader,
              &attributes_csv_loader,
              &anim_json_parser,
              &anim_code_parser,
              &metadata_provider},
          artifact_writer{
              &env.config,
              &env.config,
              env.project_root,
              &resolved.schema,
              &provider_map,
              env.text_formatter,
              env.diag.get(),
              &png_rgba_saver,
              &png_indexed_saver,
              &jasc_saver,
              &anim_json_parser,
              &anim_code_generator},
          checksum_provider{env.project_root},
          repo{
              &checksum_provider,
              &metadata_provider,
              &key_provider,
              &artifact_reader,
              &artifact_writer,
              env.diag.get()},
          compiler{
              &env.config, &resolved.schema, env.text_formatter, env.diag.get(), tile_printer.get(), pal_printer.get()}
    {
    }

    TilesetCommandServices(const TilesetCommandServices &) = delete;
    TilesetCommandServices &operator=(const TilesetCommandServices &) = delete;
    TilesetCommandServices(TilesetCommandServices &&) = delete;
    TilesetCommandServices &operator=(TilesetCommandServices &&) = delete;

    std::unique_ptr<TilePrinter> tile_printer;
    std::unique_ptr<PalettePrinter> pal_printer;
    PngRgbaImageLoader png_rgba_loader{};
    PngIndexedImageLoader png_indexed_loader{};
    PngRgbaImageSaver png_rgba_saver{};
    PngIndexedImageSaver png_indexed_saver{};
    JascPalLoader jasc_loader;
    JascPalSaver jasc_saver;
    AnimJsonParser anim_json_parser;
    AnimCodeParser anim_code_parser;
    AnimCodeGenerator anim_code_generator{};
    ProjectTilesetMetadataProvider metadata_provider;
    ProjectLayoutMetadataProvider layout_metadata_provider;
    ProjectTilesetMetadataWriter metadata_writer;
    IncbinDeclarationAppender incbin_appender;
    ProjectTilesetAnimsModifier tileset_anims_modifier;
    MetatilesHeaderProvider metatiles_header;
    TilesetAttrSchemaResolver schema_resolver;
    TilesetAttrSchemaCache schema_cache;
    // The command target's cache entry, resolved fail-fast at construction. resolved and provider_map alias into it
    // for the consumers (and commands) that operate on the target tileset only.
    const TilesetAttrSchemaCache::Entry *target_entry;
    const ResolvedTilesetAttrSchema &resolved;
    const ProviderMap &provider_map;
    ProjectPorytilesTilesetManager tileset_manager;
    AttributesCsvLoader attributes_csv_loader;
    ProjectTilesetArtifactKeyProvider key_provider;
    ProjectTilesetArtifactReader artifact_reader;
    ProjectTilesetArtifactWriter artifact_writer;
    ProjectArtifactChecksumProvider checksum_provider;
    TilesetRepo repo;
    TilesetCompiler compiler;

  private:
    [[nodiscard]] static const TilesetAttrSchemaCache::Entry *entry_or_fail(
        const TilesetAttrSchemaCache &schema_cache, const std::string &tileset_name, const UserDiagnostics &diag)
    {
        auto entry_result = schema_cache.entry(tileset_name);
        if (!entry_result.has_value()) {
            diag.fatal(entry_result);
            throw CLI::RuntimeError{1};
        }
        return entry_result.value();
    }
};

} // namespace porytiles
