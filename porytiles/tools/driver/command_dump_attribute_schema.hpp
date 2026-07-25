#pragma once

#include <cstddef>
#include <format>
#include <iostream>
#include <ostream>
#include <string>
#include <unistd.h>
#include <vector>

#include "CLI/CLI.hpp"

#include "porytiles/domain/algorithms/metatile_attribute_schema_reconciler.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/terminal_width.hpp"
#include "porytiles/utilities/text/text_wrap.hpp"
#include "porytiles/xcut/diagnostics/null_user_diagnostics.hpp"

#include "command.hpp"
#include "option.hpp"
#include "tileset_command_setup.hpp"

class DumpAttributeSchemaCommand final : public Command {
  public:
    explicit DumpAttributeSchemaCommand(CLI::App &parent_app)
        : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to resolve the schema for")->required();
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

        // Env failures report through the unfiltered stderr diagnostics: the filtered diagnostics handle is not built
        // until initialize() succeeds.
        const auto env_result = env.initialize(tileset_name_);
        if (!env_result.has_value()) {
            const auto env_fail_result = ChainableResult<void>{
                FormattableError{
                    "Failed to dump attribute schema for tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                env_result};
            env.stderr_diag.fatal(env_fail_result);
            throw CLI::RuntimeError{1};
        }

        // Verify the tileset exists in the project before resolving. The schema itself is supposed to be
        // project-global, but our config system wires all values through the full resolution path, which always allows
        // tileset-specific settings.
        if (!allow_missing_tileset_) {
            ProjectTilesetMetadataProvider metadata_provider{env.project_root, text_formatter, env.diag.get()};
            if (!metadata_provider.exists(tileset_name_)) {
                const auto not_found_err = ChainableResult<void>{FormattableError{
                    "Tileset '{}' does not exist. Pass '{}' to resolve its schema anyway.",
                    FormatParam{tileset_name_, Style::bold},
                    FormatParam{"--allow-missing-tileset", Style::bold}}};
                env.diag->fatal(not_found_err);
                throw CLI::RuntimeError{1};
            }
        }

        // Run the same resolver setup every other command uses. Its remarks and warnings go to a null sink: the
        // dump itself reports the resolved schema and its provenance on stdout, so stderr diagnostics would only
        // duplicate that and interleave with the dump.
        NullUserDiagnostics null_diag{text_formatter};
        MetatileAttributeSchemaResolver schema_resolver{env.project_root, &env.config, text_formatter, &null_diag};
        auto resolved_result = schema_resolver.resolve(tileset_name_);

        std::ostream &out = std::cout;
        const std::string section_title = "Resolved Metatile Attribute Schema";
        out << text_formatter->style(section_title, Style::bold) << "\n";
        out << text_formatter->style(std::string(section_title.size(), '='), Style::faint) << "\n\n";

        if (!resolved_result.has_value()) {
            constexpr std::size_t indent_columns = 2;
            const std::size_t wrap_width = resolve_terminal_width(STDOUT_FILENO);
            const std::size_t body_width =
                wrap_width == 0 ? 0 : (wrap_width > indent_columns ? wrap_width - indent_columns : 1);
            out << "  " << text_formatter->style("No schema could be resolved:", Style::bold) << "\n\n";
            for (const auto &err : resolved_result.chain()) {
                for (const std::string &line : err->details(*text_formatter)) {
                    if (line.empty()) {
                        continue;
                    }
                    for (const std::string &physical : wrap_ansi_line(line, body_width)) {
                        out << "  " << physical << "\n";
                    }
                }
            }
            out << "\n";
            return;
        }
        const LoadedMetatileAttributeSchema &resolved = resolved_result.value();

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
