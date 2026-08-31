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
#include "porytiles/domain/services/tileset_name_resolver.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/infra/config/cli_option_provider.hpp"
#include "porytiles/infra/config/default_provider.hpp"
#include "porytiles/infra/config/header_define_provider.hpp"
#include "porytiles/infra/config/lazy_layered_config.hpp"
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
#include "porytiles/infra/services/header_enum_map_provider.hpp"
#include "porytiles/infra/services/incbin_declaration_appender.hpp"
#include "porytiles/infra/services/jasc_palette_loader.hpp"
#include "porytiles/infra/services/jasc_palette_saver.hpp"
#include "porytiles/infra/services/metatile_attribute_schema_resolver.hpp"
#include "porytiles/infra/services/png_indexed_image_loader.hpp"
#include "porytiles/infra/services/png_indexed_image_saver.hpp"
#include "porytiles/infra/services/png_rgba_image_loader.hpp"
#include "porytiles/infra/services/png_rgba_image_saver.hpp"
#include "porytiles/infra/services/project_layout_metadata_provider.hpp"
#include "porytiles/infra/services/project_porytiles_tileset_manager.hpp"
#include "porytiles/infra/services/project_tileset_anims_modifier.hpp"
#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles/infra/services/project_tileset_metadata_writer.hpp"
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
/// and the filtered diagnostics built from the config's diagnostic include/exclude patterns.
///
/// Setup is two-phase: the constructor builds the infallible pieces, and initialize() runs the fallible half (YAML
/// validation and the diagnostic filter construction) and returns a ChainableResult the command wraps with its own
/// "Failed to <verb> tileset ..." context. Until initialize() succeeds, @c diag is unset, so failures report through
/// @c stderr_diag.
///
/// Members hold pointers into earlier members, so the class is not copyable and not movable. Commands should construct
/// it once on the stack.
class TilesetCommandEnv {
    /// @brief The layered config's provider list bundled with a typed handle to the YamlFileProvider inside it.
    ///
    /// @details
    /// make_provider_chain returns the pieces together so the constructor can hand ownership of the list to config
    /// while keeping the typed handle it needs: the YamlFileProvider for eager YAML validation. The handle points at
    /// a provider owned by the list (and, after construction, by config), so it stays valid for the env's whole
    /// lifetime.
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
        // Layered configuration, highest priority first: CLI options, YAML files, base-game header defines,
        // defaults. Every provider reads stated values only; the derivation that used to live in a provider now
        // happens in the schema resolver's reconciliation, downstream of the chain.
        //
        // TODO: YamlFileProvider holds the raw stderr sink, so its warnings bypass the user's diagnostic filters in
        // every command. The filters are themselves config values, so the chain has to exist before the filters can
        // (a bootstrap circularity). Fix by late-binding the provider's diagnostics sink to the filtered one after
        // initialize() succeeds.
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<CliOptionProvider>(cli_storage));
        providers.push_back(std::make_unique<YamlFileProvider>(text_formatter, stderr_diag, project_root));
        // Take the handle from the owning slot so its provenance is the list itself, not a moved-from local.
        auto *yaml_provider = dynamic_cast<YamlFileProvider *>(providers.back().get());
        providers.push_back(
            std::make_unique<HeaderDefineProvider>(
                project_root, std::filesystem::path{"include/fieldmap.h"}, text_formatter));
        providers.push_back(std::make_unique<DefaultProvider>());
        return {.providers = std::move(providers), .yaml_provider = yaml_provider};
    }

  public:
    TilesetCommandEnv(std::filesystem::path root, const CliOptionStorage &cli_storage)
        : project_root{std::move(root)}, injector{di::get_formatter_component, !isatty(STDERR_FILENO)},
          text_formatter{injector.get<TextFormatter *>()},
          stderr_diag{text_formatter, resolve_terminal_width(STDERR_FILENO)},
          provider_chain{make_provider_chain(project_root, cli_storage, text_formatter, &stderr_diag)},
          yaml_provider{provider_chain.yaml_provider}, config{text_formatter, std::move(provider_chain.providers)}
    {
    }

    /// @brief Runs the fallible half of env setup: YAML validation and the diagnostic filter construction.
    ///
    /// @details
    /// YAML validation failures return an error whose per-key details were already emitted through @c stderr_diag by
    /// the provider. An invalid diagnostic filter key is a real config error and fails the command too, instead of
    /// silently running with empty filters. On success, @c diag is ready for every subsequent operation.
    ///
    /// @param tileset_name The command's target tileset, used as the config scope
    /// @return Nothing on success, or the error for the command to wrap with its own context
    [[nodiscard]] ChainableResult<void> initialize(const std::string &tileset_name)
    {
        return initialize_scope(ConfigScopeType::tileset, tileset_name);
    }

    /// @brief Project-scope variant of initialize() for commands that operate on no particular tileset.
    ///
    /// @details
    /// Validates only the project-wide YAML config files and builds the diagnostic filters from project-scope config
    /// values. Used by the project config commands (e.g. dump-project-config), which resolve the provider chain
    /// without any tileset-level files.
    ///
    /// @return Nothing on success, or the error for the command to wrap with its own context
    [[nodiscard]] ChainableResult<void> initialize_project()
    {
        return initialize_scope(ConfigScopeType::project, "");
    }

  private:
    [[nodiscard]] ChainableResult<void> initialize_scope(ConfigScopeType scope_type, const std::string &scope_name)
    {
        // Eagerly validate all YAML config files for unknown keys
        if (yaml_provider->preload_and_validate(scope_type, scope_name)) {
            if (scope_type == ConfigScopeType::project) {
                return FormattableError{"Project configuration validation failed."};
            }
            return FormattableError{
                "Configuration validation failed for tileset '{}'.", FormatParam{scope_name, Style::bold}};
        }

        // Build diagnostic filters from config values
        PT_TRY_ASSIGN_PASS_ERR(warnings_exclude, config.diagnostic_warnings_exclude(scope_type, scope_name), void);
        PT_TRY_ASSIGN_PASS_ERR(warnings_include, config.diagnostic_warnings_include(scope_type, scope_name), void);
        PT_TRY_ASSIGN_PASS_ERR(remarks_exclude, config.diagnostic_remarks_exclude(scope_type, scope_name), void);
        PT_TRY_ASSIGN_PASS_ERR(remarks_include, config.diagnostic_remarks_include(scope_type, scope_name), void);

        DiagnosticTagFilter warning_filter{std::move(warnings_exclude).value(), std::move(warnings_include).value()};
        DiagnosticTagFilter remark_filter{std::move(remarks_exclude).value(), std::move(remarks_include).value()};

        // Wrap with filter decorator for all subsequent operations
        // TODO: YamlFileProvider keeps emitting through the raw stderr sink even after this point (see
        // make_provider_chain). Once it supports a late-bound diagnostics sink, rebind it to the filtered one here.
        diag = std::make_unique<FilteredUserDiagnostics>(
            text_formatter, &stderr_diag, std::move(warning_filter), std::move(remark_filter));
        return {};
    }

  public:
    TilesetCommandEnv(const TilesetCommandEnv &) = delete;
    TilesetCommandEnv &operator=(const TilesetCommandEnv &) = delete;
    TilesetCommandEnv(TilesetCommandEnv &&) = delete;
    TilesetCommandEnv &operator=(TilesetCommandEnv &&) = delete;

    std::filesystem::path project_root;
    fruit::Injector<TextFormatter> injector;
    TextFormatter *text_formatter;
    StderrStyledUserDiagnostics stderr_diag;

  private:
    // Bridges make_provider_chain's single return value across the member initializers: the handles copy out,
    // config takes ownership of the vector. After construction the providers vector is moved-from and empty.
    ProviderChain provider_chain;

  public:
    // Typed handle to the YamlFileProvider owned by config's provider list, needed for eager YAML validation.
    gsl::not_null<YamlFileProvider *> yaml_provider;
    LazyLayeredConfig config;
    std::unique_ptr<FilteredUserDiagnostics> diag;
};

/// @brief Resolves the command's tileset-name argument to the canonical name declared in the project.
///
/// @details
/// Accepts fuzzy names: "gTileset_SecretBase", "SecretBase", "secret_base", and "secretBase" all resolve to
/// "gTileset_SecretBase". This runs before @c TilesetCommandEnv::initialize() because the canonical name doubles as
/// the tileset config scope, so a failure here reports through the env's unfiltered stderr diagnostics.
///
/// @param env The command environment holding the project root and formatter
/// @param input The user-supplied tileset name argument
/// @return The canonical tileset name, or the resolution error for the command to wrap with its own context
[[nodiscard]] inline ChainableResult<std::string>
resolve_tileset_name_argument(TilesetCommandEnv &env, const std::string &input)
{
    const ProjectTilesetMetadataProvider metadata_provider{env.project_root, env.text_formatter, &env.stderr_diag};
    PT_TRY_ASSIGN_PASS_ERR(tileset_names, metadata_provider.tilesets(), std::string);
    return resolve_tileset_name(input, tileset_names, env.text_formatter);
}

/// @brief The invocation's resolved attribute schema and provider map, produced before the service graph.
///
/// @details
/// Schema resolution is the one fallible step of command setup (an ambiguous attribute size on
/// pokeemerald-expansion, a mask-set selection failure, an invalid explicit mask), so it runs before
/// TilesetCommandServices is constructed and returns a ChainableResult the command can wrap with its own
/// "Failed to <verb> tileset ..." context. The services constructor then consumes the context by value and cannot
/// fail.
struct ResolvedAttributeContext {
    LoadedMetatileAttributeSchema resolved;
    ProviderMap provider_map;
};

/// @brief Resolves the invocation's metatile attribute schema and builds the provider map for its fields.
///
/// @param env The command environment holding the config and the project root
/// @param tileset_name The command's target tileset, used as the config scope
/// @return The resolved context, or the resolver's error for the command to wrap
[[nodiscard]] inline ChainableResult<ResolvedAttributeContext>
resolve_attribute_context(TilesetCommandEnv &env, const std::string &tileset_name)
{
    MetatileAttributeSchemaResolver resolver{env.project_root, &env.config, env.text_formatter, env.diag.get()};
    PT_TRY_ASSIGN_PASS_ERR(resolved, resolver.resolve(tileset_name), ResolvedAttributeContext);
    ProviderMap provider_map =
        build_provider_map(env.project_root, resolved.schema, env.text_formatter, env.diag.get());
    return ResolvedAttributeContext{std::move(resolved), std::move(provider_map)};
}

/// @brief The service graph shared by the compile, create, import, and decompile commands.
///
/// @details
/// Consumes the resolved attribute context (the attribute layout is a project-global property, so every tileset a
/// command touches decodes with the same schema) and wires the services that consume it: the attributes CSV loader,
/// the artifact reader/writer, the tileset repo, the tileset manager, and the compiler. This is the single home for
/// that wiring so the commands cannot drift apart.
///
/// Declaration order is dependency order: the resolved schema and its provider map come before the artifact
/// reader/writer, manager, and compiler, which hold pointers into them. The service graph is not copyable, not movable,
/// and should be constructed once on the stack after the env.
///
/// Construction cannot fail: the fallible schema resolution happens in resolve_attribute_context, whose error the
/// command wraps with its own context before this graph is built.
class TilesetCommandServices {
  public:
    TilesetCommandServices(TilesetCommandEnv &env, ResolvedAttributeContext context)
        : tile_printer{std::make_unique<AsciiTilePrinter>(env.text_formatter)},
          palette_printer{std::make_unique<ColorPalettePrinter>(env.text_formatter)}, jasc_loader{env.text_formatter},
          jasc_saver{env.text_formatter}, anim_json_parser{env.text_formatter},
          anim_code_parser{env.text_formatter, env.diag.get()},
          metadata_provider{env.project_root, env.text_formatter, env.diag.get()},
          layout_metadata_provider{env.project_root, env.text_formatter, env.diag.get()},
          metadata_writer{env.project_root, env.text_formatter}, incbin_appender{env.project_root, env.text_formatter},
          tileset_anims_modifier{env.project_root, &env.config, env.diag.get()}, resolved{std::move(context.resolved)},
          provider_map{std::move(context.provider_map)},
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
              &resolved.schema,
              &provider_map,
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
              &env.config,
              &resolved.schema,
              env.text_formatter,
              env.diag.get(),
              tile_printer.get(),
              palette_printer.get()}
    {
    }

    TilesetCommandServices(const TilesetCommandServices &) = delete;
    TilesetCommandServices &operator=(const TilesetCommandServices &) = delete;
    TilesetCommandServices(TilesetCommandServices &&) = delete;
    TilesetCommandServices &operator=(TilesetCommandServices &&) = delete;

    std::unique_ptr<TilePrinter> tile_printer;
    std::unique_ptr<PalettePrinter> palette_printer;
    PngRgbaImageLoader png_rgba_loader{};
    PngIndexedImageLoader png_indexed_loader{};
    PngRgbaImageSaver png_rgba_saver{};
    PngIndexedImageSaver png_indexed_saver{};
    JascPaletteLoader jasc_loader;
    JascPaletteSaver jasc_saver;
    AnimJsonParser anim_json_parser;
    AnimCodeParser anim_code_parser;
    AnimCodeGenerator anim_code_generator{};
    ProjectTilesetMetadataProvider metadata_provider;
    ProjectLayoutMetadataProvider layout_metadata_provider;
    ProjectTilesetMetadataWriter metadata_writer;
    IncbinDeclarationAppender incbin_appender;
    ProjectTilesetAnimsModifier tileset_anims_modifier;
    // The invocation's resolved schema and its provider map, moved in from the pre-resolved context and shared by
    // every consumer below.
    LoadedMetatileAttributeSchema resolved;
    ProviderMap provider_map;
    ProjectPorytilesTilesetManager tileset_manager;
    AttributesCsvLoader attributes_csv_loader;
    ProjectTilesetArtifactKeyProvider key_provider;
    ProjectTilesetArtifactReader artifact_reader;
    ProjectTilesetArtifactWriter artifact_writer;
    ProjectArtifactChecksumProvider checksum_provider;
    TilesetRepo repo;
    TilesetCompiler compiler;
};

} // namespace porytiles
