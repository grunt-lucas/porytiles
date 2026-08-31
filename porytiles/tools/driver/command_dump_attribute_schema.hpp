#pragma once

#include <format>
#include <iostream>
#include <ostream>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles/domain/algorithms/metatile_attribute_schema_reconciler.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

#include "command.hpp"
#include "option.hpp"
#include "tileset_command_setup.hpp"

class DumpAttributeSchemaCommand final : public Command {
  public:
    explicit DumpAttributeSchemaCommand(CLI::App &parent_app)
        : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        cmd.add_option(
               "<tileset-name>", tileset_name_, "Name of the tileset to resolve the schema for (fuzzy names accepted)")
            ->required();
        cmd.add_flag(
            "--allow-missing-tileset",
            allow_missing_tileset_,
            "Resolve the schema even when the tileset does not exist, to preview what a new tileset would get.");
        project_root_opt_.RegisterOpt(cmd);
        porytiles::register_config_options(cmd, cli_storage_);
    }

  private:
    void Run() override
    {
        using namespace porytiles;

        TilesetCommandEnv env{project_root_opt_.project_root(), cli_storage_};
        auto *text_formatter = env.text_formatter;

        // The fuzzy name argument resolves to its canonical form before env initialization, since the canonical name
        // is the config scope. The schema itself is supposed to be project-global, but our config system wires all
        // values through the full resolution path, which always allows tileset-specific settings.
        // --allow-missing-tileset skips resolution (there is no declared name to resolve against) and uses the
        // argument verbatim, for previewing the schema a not-yet-created tileset would get.
        std::string tileset_name = tileset_name_;
        if (!allow_missing_tileset_) {
            auto resolved_name_result = resolve_tileset_name_argument(env, tileset_name_);
            if (!resolved_name_result.has_value()) {
                const auto fail_result = ChainableResult<void>{
                    FormattableError{
                        "Failed to dump attribute schema for tileset '{}'. Pass '{}' to resolve its schema anyway.",
                        FormatParam{tileset_name_, Style::bold},
                        FormatParam{"--allow-missing-tileset", Style::bold}},
                    resolved_name_result};
                env.stderr_diag.fatal(fail_result);
                throw CLI::RuntimeError{1};
            }
            tileset_name = std::move(resolved_name_result).value();
        }

        // Env failures report through the unfiltered stderr diagnostics: the filtered diagnostics handle is not built
        // until initialize() succeeds.
        const auto env_result = env.initialize(tileset_name);
        if (!env_result.has_value()) {
            const auto env_fail_result = ChainableResult<void>{
                FormattableError{
                    "Failed to dump attribute schema for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                env_result};
            env.stderr_diag.fatal(env_fail_result);
            throw CLI::RuntimeError{1};
        }

        // Resolve the schema with the users configured diagnostic settings. Users who opted in to all remarks may see
        // redundant output. But that's fine because:
        // 1. They opted in to noisy output, and there's an easy escape hatch
        // 2. The remark noise goes to stderr, while the dump goes to stdout, so a capturing script can choose
        MetatileAttributeSchemaResolver schema_resolver{env.project_root, &env.config, text_formatter, env.diag.get()};
        auto resolved_result = schema_resolver.resolve(tileset_name);

        // A resolution failure is the same hard error that aborts compile and import: an ambiguous attribute size, a
        // mask selection failure, or an invalid field. It reports as a fatal on stderr and exits nonzero rather than
        // printing to stdout, since there is no schema to dump and a script checking the exit code must not read the
        // failure as success.
        if (!resolved_result.has_value()) {
            const auto resolve_fail_result = ChainableResult<void>{
                FormattableError{
                    "Failed to dump attribute schema for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                resolved_result};
            env.diag->fatal(resolve_fail_result);
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
                    FormatParam{field.provider_definition().header.string()},
                    FormatParam{field.provider_definition().prefix});
            }
            std::string role_desc;
            if (field.role().has_value()) {
                role_desc = text_formatter->format("  role={}", FormatParam{to_string(field.role().value())});
            }
            out << "    "
                << text_formatter->format(
                       "{}  mask={}  offset={} width={}  default={}",
                       FormatParam{field.name(), Style::bold},
                       FormatParam{std::format("0x{:X}", field.mask())},
                       FormatParam{std::to_string(field.offset())},
                       FormatParam{std::to_string(field.width())},
                       FormatParam{std::to_string(field.default_value())})
                << provider_desc << role_desc << "\n";
        }
        if (resolved.schema.layer_type_field() == nullptr) {
            out << "    "
                << text_formatter->format(
                       "{}",
                       FormatParam{"(no field carries the layer_type role: layer types are disabled)", Style::faint})
                << "\n";
        }
        out << "\n";
    }

    static constexpr auto command_name = "dump-attribute-schema";
    static constexpr auto command_desc = "Dump the resolved metatile attribute schema for a tileset.";
    static constexpr auto command_group = "UTILITIES";
    std::string tileset_name_;
    bool allow_missing_tileset_{false};
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
