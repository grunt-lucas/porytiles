#pragma once

#include <format>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include "CLI/CLI.hpp"

#include "porytiles/domain/algorithms/metatile_attribute_schema_reconciler.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

#include "command.hpp"
#include "option.hpp"
#include "tileset_command_setup.hpp"

class DumpTilesetConfigCommand final : public Command {
  public:
    explicit DumpTilesetConfigCommand(CLI::App &parent_app)
        : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to dump config for")->required();
        project_root_opt_.RegisterOpt(cmd);
        porytiles::register_config_options(cmd, cli_storage_);
    }

  private:
    void Run() override
    {
        using namespace porytiles;

        TilesetCommandEnv env{project_root_opt_.project_root(), cli_storage_};
        auto *text_formatter = env.text_formatter;

        // Env failures report through the unfiltered stderr diagnostics: the filtered sink is not built until
        // initialize() succeeds.
        const auto env_result = env.initialize(tileset_name_);
        if (!env_result.has_value()) {
            const auto env_fail_result = ChainableResult<void>{
                FormattableError{"Failed to dump config for tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                env_result};
            env.stderr_diag.fatal(env_fail_result);
            throw CLI::RuntimeError{1};
        }

        env.config.dump_config(std::cout, ConfigScopeType::tileset, tileset_name_);

        // Resolve and print the invocation's attribute schema, mirroring the resolver setup every other command uses
        // (config fetch, fieldmap scan, inference, and reconciliation).
        MetatileAttributeSchemaResolver schema_resolver{env.project_root, &env.config, text_formatter, env.diag.get()};
        auto resolved_result = schema_resolver.resolve(tileset_name_);
        if (!resolved_result.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{
                    "Failed to resolve the metatile attribute schema for tileset '{}'.",
                    FormatParam{tileset_name_, Style::bold}},
                resolved_result};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
        const LoadedMetatileAttributeSchema &resolved = resolved_result.value();

        std::ostream &out = std::cout;
        const std::string section_title = "Resolved Metatile Attribute Schema";
        out << text_formatter->style(section_title, Style::bold) << "\n";
        out << text_formatter->style(std::string(section_title.size(), '='), Style::faint) << "\n\n";

        out << "  "
            << text_formatter->format(
                   "Attribute size: {} bytes ({})",
                   FormatParam{std::to_string(resolved.attribute_bytes), Style::bold},
                   FormatParam{resolved.size_origin})
            << "\n";

        out << "  "
            << text_formatter->format(
                   "Declaration size: {} bytes (const u{}, {})",
                   FormatParam{std::to_string(resolved.declaration_bytes), Style::bold},
                   FormatParam{std::to_string(resolved.declaration_bytes * 8)},
                   FormatParam{resolved.declaration_origin})
            << "\n";

        if (!resolved.fields_origin.empty()) {
            out << "  " << text_formatter->format("Fields from: {}", FormatParam{resolved.fields_origin, Style::bold})
                << "\n";
        }
        out << "\n";

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
        out << "\n";
    }

    static constexpr auto command_name = "dump-tileset-config";
    static constexpr auto command_desc = "Dump the full configuration provenance chain for a tileset.";
    static constexpr auto command_group = "UTILITIES";
    std::string tileset_name_;
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
