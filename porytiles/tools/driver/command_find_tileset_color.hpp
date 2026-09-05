#pragma once

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>

#include "CLI/CLI.hpp"

#include "porytiles/app/use_cases/inspect_tileset_colors.hpp"
#include "porytiles/domain/algorithms/color_search.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/infra/cli/cli_option_registration.hpp"
#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/parse_int.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

#include "command.hpp"
#include "option.hpp"
#include "tileset_command_setup.hpp"
#include "validators.hpp"

/// @brief Locates every pixel of a given color in a tileset's Porytiles assets.
///
/// @details
/// A debugging aid to help when you have "one stray pixel" that you just can't find. Prints each metatile layer and
/// animation tile that contains the color, rendered as ASCII art with the matching pixels marked.
class FindTilesetColorCommand final : public Command {
  public:
    explicit FindTilesetColorCommand(CLI::App &parent_app)
        : Command{parent_app, command_name, command_desc, command_group}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to search (fuzzy names accepted)")
            ->required();
        cmd.add_option(
               "<color>",
               color_,
               "Color to find, as R,G,B with each component in 0-255 (an alpha component is ignored)")
            ->required()
            ->check(Rgba32ColorValidator{});
        cmd.add_option(
               "--limit",
               limit_,
               "Maximum number of matching metatile layers and animation tiles to render, or 'all' for no cap.")
            ->check(MatchLimitValidator{})
            ->capture_default_str();
        cmd.add_option(
               "--tolerance",
               tolerance_,
               "Match rule. An integer N matches a pixel when each of its R, G, and B is within N of the requested "
               "color (0 is an exact match). 'gba' matches every pixel the GBA displays as the same color as the "
               "requested one, i.e. each of R, G, and B divided by 8 gives the same 5-bit value.")
            ->check(ColorToleranceValidator{})
            ->capture_default_str();
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
                FormattableError{"Failed to search tileset '{}'.", FormatParam{tileset_name_, Style::bold}},
                resolved_name_result};
            env.stderr_diag.fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
        const std::string tileset_name = std::move(resolved_name_result).value();

        const auto env_result = env.initialize(tileset_name);
        if (!env_result.has_value()) {
            const auto env_fail_result = ChainableResult<void>{
                FormattableError{"Failed to search tileset '{}'.", FormatParam{tileset_name, Style::bold}}, env_result};
            env.stderr_diag.fatal(env_fail_result);
            throw CLI::RuntimeError{1};
        }

        auto attribute_context = resolve_attribute_context(env, tileset_name);
        if (!attribute_context.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to search tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                attribute_context};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }
        TilesetCommandServices services{env, std::move(attribute_context).value()};

        // These arguments passed their CLI11 validators, so the parses cannot fail.
        const auto color = parse_rgba32_string(color_);
        assert_or_panic(color.has_value(), "color argument passed validation but failed to parse");
        const auto tolerance = parse_color_tolerance(tolerance_);
        assert_or_panic(tolerance.has_value(), "tolerance argument passed validation but failed to parse");
        std::optional<std::size_t> limit{};
        if (limit_ != "all") {
            const auto parsed_limit = parse_int<std::size_t>(limit_, 10);
            assert_or_panic(parsed_limit.has_value(), "limit argument passed validation but failed to parse");
            limit = parsed_limit.value();
        }

        const InspectTilesetColors inspect_use_case{
            &services.repo,
            &services.metadata_provider,
            &services.tileset_manager,
            &env.config,
            env.text_formatter,
            services.tile_printer.get(),
            services.palette_printer.get(),
            env.diag.get()};
        const FindColorOptions options{.color = color.value(), .tolerance = tolerance.value(), .limit = limit};
        auto lines_result = inspect_use_case.find_color(tileset_name, options);
        if (!lines_result.has_value()) {
            const auto fail_result = ChainableResult<void>{
                FormattableError{"Failed to search tileset '{}'.", FormatParam{tileset_name, Style::bold}},
                lines_result};
            env.diag->fatal(fail_result);
            throw CLI::RuntimeError{1};
        }

        for (const auto &line : lines_result.value()) {
            std::cout << line << "\n";
        }
    }

    static constexpr auto command_name = "find-tileset-color";
    static constexpr auto command_desc =
        "Find every metatile and animation tile containing a color, with the matching pixels marked.";
    static constexpr auto command_group = "UTILITIES";
    std::string tileset_name_;
    std::string color_;
    std::string limit_{"10"};
    std::string tolerance_{"0"};
    OptProjectRoot project_root_opt_;
    porytiles::CliOptionStorage cli_storage_;
};
