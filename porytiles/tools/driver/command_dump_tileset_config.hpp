#pragma once

#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <unistd.h>
#include <vector>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles/utilities/result/chainable_result.hpp"

#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/domain/services/metatile_attr_schema_loader.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/infra/config/cli_option_provider.hpp"
#include "porytiles/infra/config/default_provider.hpp"
#include "porytiles/infra/config/header_define_provider.hpp"
#include "porytiles/infra/config/lazy_layered_config.hpp"
#include "porytiles/infra/config/metatile_attribute_config_provider.hpp"
#include "porytiles/infra/config/metatiles_header_provider.hpp"
#include "porytiles/infra/config/yaml_file_provider.hpp"
#include "porytiles/infra/services/project_layout_metadata_provider.hpp"
#include "porytiles/infra/services/tileset_attr_schema_resolver.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"
#include "porytiles/xcut/di/components.hpp"
#include "porytiles/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

#include "command.hpp"
#include "option.hpp"

class DumpTilesetConfigCommand final : public Command {
  public:
    explicit DumpTilesetConfigCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to dump config for")->required();
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

        config.dump_config(std::cout, ConfigScopeType::tileset, tileset_name_);

        // Resolve and print the per-tileset attribute schema, mirroring the resolver setup every other command uses
        // (size detection from metatiles.h, layout selection, and mask-driven widening).
        ProjectLayoutMetadataProvider layout_metadata_provider{project_root, text_formatter, stderr_diag.get()};
        MetatilesHeaderProvider metatiles_header{project_root, text_formatter};
        TilesetAttrSchemaResolver schema_resolver{
            &config, &layout_metadata_provider, &metatiles_header, text_formatter, stderr_diag.get()};
        auto resolved_result = schema_resolver.resolve(tileset_name_);
        if (!resolved_result.has_value()) {
            stderr_diag->fatal(resolved_result);
            throw CLI::RuntimeError{1};
        }
        const ResolvedTilesetAttrSchema &resolved = resolved_result.value();

        std::ostream &out = std::cout;
        const std::string section_title = "Resolved Metatile Attribute Schema";
        out << text_formatter->style(section_title, Style::bold) << "\n";
        out << text_formatter->style(std::string(section_title.size(), '='), Style::faint) << "\n\n";

        out << "  "
            << text_formatter->format("Layout: {}", FormatParam{to_string(resolved.layout), Style::cyan | Style::bold})
            << "\n";

        out << "  "
            << text_formatter->format(
                   "Attribute size: {} bytes", FormatParam{std::to_string(resolved.attr_bytes), Style::bold})
            << "\n\n";

        out << "  " << text_formatter->style("Fields:", Style::faint) << "\n";
        for (const Field &field : resolved.schema.fields()) {
            std::string provider_desc;
            if (field.has_provider()) {
                provider_desc = text_formatter->format(
                    " provider={} ({})",
                    FormatParam{field.provider_spec().header.string()},
                    FormatParam{field.provider_spec().prefix});
            }
            out << "    "
                << text_formatter->format(
                       "{}  mask={}  offset={} width={}  default={}",
                       FormatParam{field.name(), Style::bold},
                       FormatParam{std::format("0x{:X}", field.mask())},
                       FormatParam{std::to_string(field.offset())},
                       FormatParam{std::to_string(field.width())},
                       FormatParam{std::to_string(field.default_value())})
                << provider_desc << "\n";
        }

        // Fields excluded for the chosen layout: they carry a mask for the other layout but not this one.
        const bool frlg = resolved.layout == AttrSchemaLayout::frlg;
        std::vector<std::string> excluded;
        for (const auto &spec : resolved.resolved_specs) {
            const bool has_selected = frlg ? spec.frlg_mask.has_value() : spec.mask.has_value();
            if (!has_selected) {
                excluded.push_back(spec.name);
            }
        }
        if (!excluded.empty()) {
            std::string joined;
            for (std::size_t i = 0; i < excluded.size(); ++i) {
                joined += (i == 0 ? "" : ", ") + excluded.at(i);
            }
            out << "\n  " << text_formatter->style("Excluded for this layout: " + joined, Style::faint) << "\n";
        }
        out << "\n";
    }

  private:
    static constexpr auto kCommandName = "dump-tileset-config";
    static constexpr auto kCommandDesc = "Dump the full configuration provenance chain for a tileset.";
    static constexpr auto kCommandGroup = "UTILITIES";
    std::string tileset_name_;
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
