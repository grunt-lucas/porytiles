#pragma once

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles/app/use_cases/inspect_tileset_colors.hpp"
#include "porytiles/domain/algorithms/color_search.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

#include "command.hpp"
#include "option.hpp"
#include "tileset_command_setup.hpp"
#include "validators.hpp"

/// @brief Lists every color in a tileset's Porytiles assets with pixel counts.
///
/// @details
/// The companion to find-tileset-color, helps with finding a stray color when it's not known. The list is sorted by
/// pixel count, so a color with a handful of pixels stands out at the tail, and the unique color total is compared
/// against the configured color limits. With --group, colors the GBA displays as one color cluster together (or colors
/// within a per-channel tolerance, when --tolerance is an integer).
class DumpTilesetColorsCommand final : public Command {
  public:
    explicit DumpTilesetColorsCommand(CLI::App &parent_app)
        : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to dump colors for (fuzzy names accepted)")
            ->required();
        auto *group_opt = cmd.add_flag(
            "--group",
            group_,
            "Cluster similar colors under their most common member instead of printing a flat list.");
        cmd.add_option(
               "--tolerance",
               tolerance_,
               "Grouping rule for --group. 'gba' (the default) groups colors the GBA displays as one color, i.e. each "
               "of R, G, and B divided by 8 gives the same 5-bit value, so every group can be merged with no visible "
               "change. An integer N instead groups colors whose R, G, and B each differ by at most N.")
            ->check(ColorToleranceValidator{})
            ->needs(group_opt);
        project_root_opt_.RegisterOpt(cmd);
        porytiles::register_config_options(cmd, cli_storage_);
    }

  private:
    void Run() override
    {
        using namespace porytiles;

        TilesetCommandEnv env{project_root_opt_.project_root(), cli_storage_};

        // The fuzzy name argument must be resolved to its canonical form first, since the canonical name is the config
        // scope for env initialization and the key for every later lookup. This resolution also guarantees the tileset
        // existence.
        auto resolved_name_result = resolve_tileset_name_argument(env, tileset_name_);
        if (!resolved_name_result.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to dump colors for tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                resolved_name_result};
            env.stderr_diag.fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
        const std::string tileset_name = std::move(resolved_name_result).value();

        const auto env_result = env.initialize(tileset_name);
        if (!env_result.has_value()) {
            const auto env_fail_result = ChainableResult<void>{
                FormattableError{"Failed to dump colors for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                env_result};
            env.stderr_diag.fatal(env_fail_result);
            throw CLI::RuntimeError{1};
        }

        auto attribute_context = resolve_attribute_context(env, tileset_name);
        if (!attribute_context.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to dump colors for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                attribute_context};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
        TilesetCommandServices services{env, std::move(attribute_context).value()};

        const InspectTilesetColors inspect_use_case{
            &services.repo,
            &services.metadata_provider,
            &services.tileset_manager,
            &env.config,
            env.text_formatter,
            services.tile_printer.get(),
            services.palette_printer.get(),
            env.diag.get()};
        std::optional<ColorTolerance> tolerance{};
        if (tolerance_.has_value()) {
            // The argument passed its CLI11 validator, so the parse cannot fail.
            const auto parsed = parse_color_tolerance(tolerance_.value());
            assert_or_panic(parsed.has_value(), "tolerance argument passed validation but failed to parse");
            tolerance = parsed.value();
        }
        const DumpColorsOptions options{.group = group_, .tolerance = tolerance};
        auto lines_result = inspect_use_case.dump_colors(tileset_name, options);
        if (!lines_result.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to dump colors for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                lines_result};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }

        for (const auto &line : lines_result.value()) {
            std::cout << line << "\n";
        }
    }

    static constexpr auto command_name = "dump-tileset-colors";
    static constexpr auto command_desc =
        "Dump every color in a tileset with pixel counts, compared against the color budget.";
    static constexpr auto command_group = "UTILITIES";
    std::string tileset_name_;
    bool group_{false};
    std::optional<std::string> tolerance_{};
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
