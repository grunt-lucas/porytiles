#include "legacy/cli_parser.h"

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest.h>
#endif // DOCTEST_CONFIG_DISABLE

#include <fmt/color.h>
#include <getopt.h>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>

#include "build_version.h"
#include "legacy/cli_options.h"
#include "legacy/logger.h"
#include "legacy/palette_assignment.h"
#include "legacy/porytiles_exception.h"
#include "legacy/utilities.h"
#include "panic/panic.hpp"

namespace porytiles {

static void parseGlobalOptions(PorytilesContext &ctx, int argc, char *const *argv);

static void parseSubcommand(PorytilesContext &ctx, int argc, char *const *argv);

static void parseSubcommandOptions(PorytilesContext &ctx, int argc, char *const *argv);

/*
 * Help menu strings
 */
// @formatter:off
// clang-format off
const std::string COMPILE_PRIMARY_COMMAND = "compile-primary";
const std::string COMPILE_SECONDARY_COMMAND = "compile-secondary";
const std::string DECOMPILE_PRIMARY_COMMAND = "decompile-primary";
const std::string DECOMPILE_SECONDARY_COMMAND = "decompile-secondary";

const std::string GLOBAL_HELP = std::string{fmt::format(R"(
porytiles {} {}
grunt-lucas <grunt.lucas@yahoo.com>

Overworld tileset compiler for use with the pokeruby, pokefirered, and
pokeemerald Pokémon Generation III decompilation projects from pret. Also
compatible with pokeemerald-expansion from rh-hideout. Builds Porymap-ready
tilesets from RGBA (or indexed) tile assets.

Project home page: https://github.com/grunt-lucas/porytiles


USAGE
    porytiles [GLOBAL OPTIONS] SUBCOMMAND [OPTIONS] [ARGS ...]
    porytiles --help
    porytiles --version

GLOBAL OPTIONS
{}
{}
{}

SUBCOMMANDS
    {}
        Compile a primary tileset. All files are generated in-place at the
        output location. Compilation transforms RGBA (or indexed) tile assets
        into a Porymap-ready tileset.

    {}
        Compile a secondary tileset. All files are generated in-place at the
        output location. Compilation transforms RGBA (or indexed) tile assets
        into a Porymap-ready tileset.

    {}
        Decompile a primary tileset. All files are generated in-place at the
        output location. Decompilation transforms a Porymap-ready tileset into
        RGBA tile assets.

    {}
        Decompile a secondary tileset. All files are generated in-place at the
        output location. Decompilation transforms a Porymap-ready tileset into
        RGBA tile assets.

Run `porytiles SUBCOMMAND --help' for more information about a subcommand and
its OPTIONS and ARGS.

To get more help with Porytiles, check out the guides at:
    https://github.com/grunt-lucas/porytiles/wiki
    https://www.youtube.com/playlist?list=PLuyjFojPxF7-O5o_mS6uTBtyYcuyFf_Ce

SEE ALSO
    https://github.com/pret/pokeruby
    https://github.com/pret/pokefirered
    https://github.com/pret/pokeemerald
    https://github.com/rh-hideout/pokeemerald-expansion
    https://github.com/huderlem/porymap
)",
std::string{PORYTILES_BUILD_VERSION}, std::string{PORYTILES_BUILD_DATE}, HELP_DESC, VERBOSE_DESC, VERSION_DESC,
COMPILE_PRIMARY_COMMAND, COMPILE_SECONDARY_COMMAND, DECOMPILE_PRIMARY_COMMAND, DECOMPILE_SECONDARY_COMMAND
)}.substr(1);

const std::string COMPILATION_INPUT_DIRECTORY_FORMAT = std::string{fmt::format(R"(
    Compilation Input Directory Format
        The compilation input directory must conform to the following format.
        `[]' indicates optional assets.
            src/
                # bottom metatile layer (RGBA, 8-bit, or 16-bit indexed)
                bottom.png
                # middle metatile layer (RGBA, 8-bit, or 16-bit indexed)
                middle.png
                # top metatile layer (RGBA, 8-bit, or 16-bit indexed)
                top.png
                # cached configuration for palette assignment algorithm
                [assign.cache]
                # missing metatile entries will receive default values
                [attributes.csv]
                [anim/]
                    # animation names can be arbitrary, but must be unique
                    [anim1/]
                        # you must specify a key frame PNG for each anim
                        key.png
                        # you must specify at least a 00.png anim frame
                        00.png
                        # frames must be named numerically, in order
                        [01.png]
                        # you may specify an arbitrary number of frames
                        ...
                    # you may specify an arbitrary number of animations
                    ...
                # `palette-primers' folder is optional
                [palette-primers]
                    # e.g. a pal file containing all the colors for your foliage
                    [foliage.pal]
                    # you may specify an arbitrary number of primer palettes
                    ...
                # `palette-overrides' folder is optional
                [palette-overrides]
                    # e.g. a pal file containing overrides for 01.pal
                    [01.pal]
                    # you may specify more override palettes for other pal indexes
                    ...
)"
)}.substr(1);

const std::string DECOMPILATION_INPUT_DIRECTORY_FORMAT = std::string{fmt::format(R"(
    Decompilation Input Directory Format\n"
        The decompilation input directory must conform to the following format.
        `[]' indicates optional assets.
            bin/
                # binary file containing attributes of each metatile
                metatile_attributes.bin
                # binary file containing metatile entries
                metatiles.bin
                # indexed png of raw tiles
                tiles.png
                # directory of palette files
                palettes
                    # JASC pal file for palette 0
                    00.pal
                    # number of pal files must match base game pals total count
                    ...
                # `anim' folder is optional
                [anim/]
                    # animation names can be arbitrary, but must be unique
                    [anim1/]
                        # you must specify at least a 00.png anim frame
                        00.png
                        # frames must be named numerically, in order
                        [01.png]
                        # you may specify an arbitrary number of frames
                        ...
                    # you may specify an arbitrary number of animations
                    ...
)"
)}.substr(1);

const std::string WARN_OPTIONS_HEADER = std::string{fmt::format(R"(
        Use these options to enable or disable additional warnings, as well as
        set specific warnings as errors. For more information and a full list of
        available warnings, check:
        https://github.com/grunt-lucas/porytiles/wiki/Warnings-and-Errors
)"
)}.substr(1);

const std::string COMPILE_PRIMARY_HELP = std::string{fmt::format(R"(
USAGE
    porytiles {} [OPTIONS] INPUT-PATH BEHAVIORS-HEADER

Compile RGBA tile assets into a Porymap-ready primary tileset. `compile-primary'
expects an input path containing the target assets organized according to the
format outlined in the Compilation Input Directory Format subsection. You must
also supply your project's `metatile_behaviors.h' file. By default,
`compile-primary' will write output to the current working directory, but you
can change this behavior by supplying the `-o' option.

ARGS
    <INPUT-PATH>
        Path to a directory containing the RGBA tile assets for the target
        primary set. The directory must conform to the Compilation Input
        Directory Format outlined below. This tileset is the `target tileset.'

    <BEHAVIORS-HEADER>
        Path to your project's `metatile_behaviors.h' file. This file is likely
        located in your project's `include/constants' folder.

{}
OPTIONS
    For more detailed information about the options below, check out the options
    pages here:
      https://github.com/grunt-lucas/porytiles/wiki#advanced-topics

    Driver Options
{}
{}
{}
{}
    Tileset Compilation Options
{}
{}
{}
{}
{}
{}
    Palette Assignment Config Options
{}
{}
{}
    Fieldmap Override Options
{}
{}
{}
{}
{}
{}
    Warning Options
{}
{}
{}
{}
{}
)",
COMPILE_PRIMARY_COMMAND, COMPILATION_INPUT_DIRECTORY_FORMAT,
// Driver options
OUTPUT_DESC, TILES_OUTPUT_PAL_DESC, DISABLE_METATILE_GENERATION_DESC, DISABLE_ATTRIBUTE_GENERATION_DESC,
// Tileset compilation options
TARGET_BASE_GAME_DESC, DUAL_LAYER_DESC, TRANSPARENCY_COLOR_DESC, DEFAULT_BEHAVIOR_DESC, DEFAULT_ENCOUNTER_TYPE_DESC, DEFAULT_TERRAIN_TYPE_DESC,
// Palette assignment config options
ASSIGN_ALGO_DESC, EXPLORE_CUTOFF_DESC, BEST_BRANCHES_DESC,
// Fieldmap override options
TILES_PRIMARY_OVERRIDE_DESC, TILES_TOTAL_OVERRIDE_DESC, METATILES_PRIMARY_OVERRIDE_DESC, METATILES_TOTAL_OVERRIDE_DESC, PALS_PRIMARY_OVERRIDE_DESC, PALS_TOTAL_OVERRIDE_DESC,
// Warning options
WARN_OPTIONS_HEADER, WALL_DESC, WNONE_DESC, W_GENERAL_DESC, WERROR_DESC
)}.substr(1);

const std::string COMPILE_SECONDARY_HELP = std::string{fmt::format(R"(
USAGE
    porytiles {} [OPTIONS] INPUT-PATH PRIMARY-INPUT-PATH BEHAVIORS-HEADER

Compile RGBA tile assets into a Porymap-ready secondary tileset.
`compile-secondary' expects an input path containing the target assets organized
according to the format outlined in the Compilation Input Directory Format
subsection. You must also supply the RGBA tile assets for a paired primary
tileset, so Porytiles can take advantage of the Generation III engine's tile
re-use system. Like `compile-primary', you must also supply your project's
`metatile_behaviors.h' file. By default, `compile-secondary' will write output
to the current working directory, but you can change this behavior by supplying
the `-o' option.

ARGS
    <INPUT-PATH>
        Path to a directory containing the RGBA tile assets for the target
        secondary set. The directory must conform to the Compilation Input
        Directory Format outlined below. This tileset is the `target tileset.'

    <PRIMARY-INPUT-PATH>
        Path to a directory containing the RGBA tile assets for the paired
        primary set of the target secondary tileset. The directory must conform
        to the Compilation Input Directory Format outlined below.

    <BEHAVIORS-HEADER>
        Path to your project's `metatile_behaviors.h' file. This file is likely
        located in your project's `include/constants' folder.

{}
OPTIONS
    For more detailed information about the options below, check out the options
    pages here:
      https://github.com/grunt-lucas/porytiles/wiki#advanced-topics

    Driver Options
{}
{}
{}
{}
    Tileset Compilation Options
{}
{}
{}
{}
{}
{}
    Palette Assignment Config Options
{}
{}
{}
    Primary Palette Assignment Config Options
{}
{}
{}
    Fieldmap Override Options
{}
{}
{}
{}
{}
{}
    Warning Options
{}
{}
{}
{}
{}
)",
COMPILE_SECONDARY_COMMAND, COMPILATION_INPUT_DIRECTORY_FORMAT,
// Driver options
OUTPUT_DESC, TILES_OUTPUT_PAL_DESC, DISABLE_METATILE_GENERATION_DESC, DISABLE_ATTRIBUTE_GENERATION_DESC,
// Tileset compilation options
TARGET_BASE_GAME_DESC, DUAL_LAYER_DESC, TRANSPARENCY_COLOR_DESC, DEFAULT_BEHAVIOR_DESC, DEFAULT_ENCOUNTER_TYPE_DESC, DEFAULT_TERRAIN_TYPE_DESC,
// Palette assignment config options
ASSIGN_ALGO_DESC, EXPLORE_CUTOFF_DESC, BEST_BRANCHES_DESC,
// Primary palette assignment config options
PRIMARY_ASSIGN_ALGO_DESC, PRIMARY_EXPLORE_CUTOFF_DESC, PRIMARY_BEST_BRANCHES_DESC,
// Fieldmap override options
TILES_PRIMARY_OVERRIDE_DESC, TILES_TOTAL_OVERRIDE_DESC, METATILES_PRIMARY_OVERRIDE_DESC, METATILES_TOTAL_OVERRIDE_DESC, PALS_PRIMARY_OVERRIDE_DESC, PALS_TOTAL_OVERRIDE_DESC,
// Warning options
WARN_OPTIONS_HEADER, WALL_DESC, WNONE_DESC, W_GENERAL_DESC, WERROR_DESC
)}.substr(1);

const std::string DECOMPILE_PRIMARY_HELP = std::string{fmt::format(R"(
USAGE
    porytiles {} [OPTIONS] INPUT-PATH BEHAVIORS-HEADER

Decompile a Porymap-ready primary tileset back into Porytiles-compatible RGBA
tile assets. `decompile-primary' expects an input path containing target
compiled tile assets organized according to the format outlined in the
Decompilation Input Directory Format subsection. Like the compilation commands,
`decompile-primary' requires your project's `metatile_behaviors.h' file. You can
control its output location via the `-o' option.

ARGS
    <INPUT-PATH>
        Path to a directory containing the compiled primary tileset. The
        directory must conform to the Decompilation Input Directory Format
        outlined below. This tileset is the `target tileset.'

    <BEHAVIORS-HEADER>
        Path to your project's `metatile_behaviors.h' file. This file is likely
        located in your project's `include/constants' folder.

{}
OPTIONS
    For more detailed information about the options below, check out the options
    pages here:
      https://github.com/grunt-lucas/porytiles/wiki#advanced-topics

    Driver Options
{}
    Tileset Decompilation Options
{}
{}
{}
    Fieldmap Override Options
{}
{}
{}
{}
    Warning Options
{}
{}
{}
{}
{}
)",
DECOMPILE_PRIMARY_COMMAND, DECOMPILATION_INPUT_DIRECTORY_FORMAT,
// Driver options
OUTPUT_DESC,
// Tileset decompilation options
TARGET_BASE_GAME_DESC, NORMALIZE_TRANSPARENCY_DESC, PRESERVE_TRANSPARENCY_DESC,
// Fieldmap override options
TILES_PRIMARY_OVERRIDE_DESC, TILES_TOTAL_OVERRIDE_DESC, PALS_PRIMARY_OVERRIDE_DESC, PALS_TOTAL_OVERRIDE_DESC,
// Warning options
WARN_OPTIONS_HEADER, WALL_DESC, WNONE_DESC, W_GENERAL_DESC, WERROR_DESC
)}.substr(1);

const std::string DECOMPILE_SECONDARY_HELP = std::string{fmt::format(R"(
USAGE
    porytiles {} [OPTIONS] INPUT-PATH PRIMARY-INPUT-PATH BEHAVIORS-HEADER

Decompile a Porymap-ready secondary tileset back into Porytiles-compatible RGBA
tile assets. `decompile-secondary' expects an input path containing target
compiled tile assets organized according to the format outlined in the
Decompilation Input Directory Format subsection. You must also supply the
compiled tile assets of the target tileset's paired primary.
`decompile-secondary' requires your project's `metatile_behaviors.h' file. You
can control its output location via the `-o' option.

ARGS
    <INPUT-PATH>
        Path to a directory containing the compiled secondary tileset. The
        directory must conform to the Decompilation Input Directory Format
        outlined below. This tileset is the `target tileset.'

    <PRIMARY-INPUT-PATH>
        Path to a directory containing the compiled paired primary tileset for
        the target secondary tileset. The directory must conform to the
        Decompilation Input Directory Format outlined below.

    <BEHAVIORS-HEADER>
        Path to your project's `metatile_behaviors.h' file. This file is likely
        located in your project's `include/constants' folder.

{}
OPTIONS
    For more detailed information about the options below, check out the options
    pages here:
      https://github.com/grunt-lucas/porytiles/wiki#advanced-topics

    Driver Options
{}
    Tileset Decompilation Options
{}
{}
{}
    Fieldmap Override Options
{}
{}
{}
{}
    Warning Options
{}
{}
{}
{}
{}
)",
DECOMPILE_SECONDARY_COMMAND, DECOMPILATION_INPUT_DIRECTORY_FORMAT,
// Driver options
OUTPUT_DESC,
// Tileset decompilation options
TARGET_BASE_GAME_DESC, NORMALIZE_TRANSPARENCY_DESC, PRESERVE_TRANSPARENCY_DESC,
// Fieldmap override options
TILES_PRIMARY_OVERRIDE_DESC, TILES_TOTAL_OVERRIDE_DESC, PALS_PRIMARY_OVERRIDE_DESC, PALS_TOTAL_OVERRIDE_DESC,
// Warning options
WARN_OPTIONS_HEADER, WALL_DESC, WNONE_DESC, W_GENERAL_DESC, WERROR_DESC
)}.substr(1);
// @formatter:on
// clang-format on

// TODO : add an EXAMPLES section to each help menu

std::unordered_map<std::string, std::unordered_set<Subcommand>> supportedSubcommands = {
    {HELP,
     {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY, Subcommand::DECOMPILE_PRIMARY,
      Subcommand::DECOMPILE_SECONDARY}},
    {OUTPUT,
     {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY, Subcommand::DECOMPILE_PRIMARY,
      Subcommand::DECOMPILE_SECONDARY}},
    {TILES_OUTPUT_PAL, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {DISABLE_METATILE_GENERATION, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {DISABLE_ATTRIBUTE_GENERATION, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {TARGET_BASE_GAME,
     {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY, Subcommand::DECOMPILE_PRIMARY,
      Subcommand::DECOMPILE_SECONDARY}},
    {DUAL_LAYER, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {TRANSPARENCY_COLOR, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {DEFAULT_BEHAVIOR, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {DEFAULT_ENCOUNTER_TYPE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {DEFAULT_TERRAIN_TYPE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {NORMALIZE_TRANSPARENCY, {Subcommand::DECOMPILE_PRIMARY, Subcommand::DECOMPILE_SECONDARY}},
    {PRESERVE_TRANSPARENCY, {Subcommand::DECOMPILE_PRIMARY, Subcommand::DECOMPILE_SECONDARY}},
    {ASSIGN_ALGO, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {EXPLORE_CUTOFF, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {BEST_BRANCHES, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {PRIMARY_ASSIGN_ALGO, {Subcommand::COMPILE_SECONDARY}},
    {PRIMARY_EXPLORE_CUTOFF, {Subcommand::COMPILE_SECONDARY}},
    {PRIMARY_BEST_BRANCHES, {Subcommand::COMPILE_SECONDARY}},
    {TILES_PRIMARY_OVERRIDE,
     {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY, Subcommand::DECOMPILE_PRIMARY,
      Subcommand::DECOMPILE_SECONDARY}},
    {TILES_TOTAL_OVERRIDE,
     {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY, Subcommand::DECOMPILE_PRIMARY,
      Subcommand::DECOMPILE_SECONDARY}},
    {METATILES_PRIMARY_OVERRIDE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {METATILES_TOTAL_OVERRIDE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {PALS_PRIMARY_OVERRIDE,
     {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY, Subcommand::DECOMPILE_PRIMARY,
      Subcommand::DECOMPILE_SECONDARY}},
    {PALS_TOTAL_OVERRIDE,
     {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY, Subcommand::DECOMPILE_PRIMARY,
      Subcommand::DECOMPILE_SECONDARY}},
    {WALL,
     {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY, Subcommand::DECOMPILE_PRIMARY,
      Subcommand::DECOMPILE_SECONDARY}},
    {WNONE,
     {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY, Subcommand::DECOMPILE_PRIMARY,
      Subcommand::DECOMPILE_SECONDARY}},
    {WNO_ERROR,
     {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY, Subcommand::DECOMPILE_PRIMARY,
      Subcommand::DECOMPILE_SECONDARY}},
    {WERROR,
     {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY, Subcommand::DECOMPILE_PRIMARY,
      Subcommand::DECOMPILE_SECONDARY}},
    // Compilation warnings
    {WCOLOR_PRECISION_LOSS, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_COLOR_PRECISION_LOSS, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WKEY_FRAME_NO_MATCHING_TILE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_KEY_FRAME_NO_MATCHING_TILE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WATTRIBUTE_FORMAT_MISMATCH, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_ATTRIBUTE_FORMAT_MISMATCH, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WUNUSED_ATTRIBUTE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_UNUSED_ATTRIBUTE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WTRANSPARENCY_COLLAPSE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_TRANSPARENCY_COLLAPSE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WKEY_FRAME_MISSING_COLORS, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_KEY_FRAME_MISSING_COLORS, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WUNUSED_MANUAL_PAL_COLOR, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_UNUSED_MANUAL_PAL_COLOR, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    // Decompilation warnings
    {WTILE_INDEX_OUT_OF_RANGE, {Subcommand::DECOMPILE_PRIMARY, Subcommand::DECOMPILE_SECONDARY}},
    {WNO_TILE_INDEX_OUT_OF_RANGE, {Subcommand::DECOMPILE_PRIMARY, Subcommand::DECOMPILE_SECONDARY}},
    {WPALETTE_INDEX_OUT_OF_RANGE, {Subcommand::DECOMPILE_PRIMARY, Subcommand::DECOMPILE_SECONDARY}},
    {WNO_PALETTE_INDEX_OUT_OF_RANGE, {Subcommand::DECOMPILE_PRIMARY, Subcommand::DECOMPILE_SECONDARY}},
    // TODO : this does not correctly handle the -Werror=foo case where foo is an incompatible warning
};

void parseOptions(PorytilesContext &ctx, int argc, char *const *argv) {
    parseGlobalOptions(ctx, argc, argv);
    parseSubcommand(ctx, argc, argv);

    switch (ctx.subcommand) {
    case Subcommand::DECOMPILE_PRIMARY:
    case Subcommand::DECOMPILE_SECONDARY:
    case Subcommand::COMPILE_PRIMARY:
    case Subcommand::COMPILE_SECONDARY:
        parseSubcommandOptions(ctx, argc, argv);
        break;
    default:
        Panic("cli_parser::parseOptions unknown subcommand setting");
    }
}

template <typename T>
static T parseIntegralOption(const PorytilesContext &ctx, const std::string &optionName, const char *optarg) {
    try {
        T arg = parseInteger<T>(optarg);
        return arg;
    } catch (const std::exception &e) {
        const auto msg = fmt::format("invalid argument '{}' for option '{}': {}", ctx.diag->Bold(optarg),
                                     ctx.diag->Bold(optionName), e.what());
        ctx.diag->Report(kFatalGeneric, msg);
        throw PorytilesException{msg};
    }
    // unreachable, here for compiler
    throw std::runtime_error("cli_parser::parseIntegralOption reached unreachable code path");
}

static RGBA32 parseRgbColor(const PorytilesContext &ctx, std::string optionName, const std::string &colorString) {
    std::vector<std::string> colorComponents = split(colorString, ",");
    if (colorComponents.size() != 3) {
        const auto msg = fmt::format("invalid argument '{}' for option '{}': RGB color must have three components",
                                     ctx.diag->Bold(colorString), ctx.diag->Bold(optionName));
        ctx.diag->Report(kFatalGeneric, msg);
        throw PorytilesException{msg};
    }
    int red = parseIntegralOption<int>(ctx, optionName, colorComponents[0].c_str());
    int green = parseIntegralOption<int>(ctx, optionName, colorComponents[1].c_str());
    int blue = parseIntegralOption<int>(ctx, optionName, colorComponents[2].c_str());

    if (red < 0 || red > 255) {
        const auto msg = fmt::format("invalid red component '{}' for option '{}': range must be 0 <= red <= 255",
                                     ctx.diag->Bold(red), ctx.diag->Bold(optionName));
        ctx.diag->Report(kFatalGeneric, msg);
        throw PorytilesException{msg};
    }
    if (green < 0 || green > 255) {
        const auto msg = fmt::format("invalid green component '{}' for option '{}': range must be 0 <= green <= 255",
                                     ctx.diag->Bold(green), ctx.diag->Bold(optionName));
        ctx.diag->Report(kFatalGeneric, msg);
        throw PorytilesException{msg};
    }
    if (blue < 0 || blue > 255) {
        const auto msg = fmt::format("invalid blue component '{}' for option '{}': range must be 0 <= blue <= 255",
                                     ctx.diag->Bold(blue), ctx.diag->Bold(optionName));
        ctx.diag->Report(kFatalGeneric, msg);
        throw PorytilesException{msg};
    }

    return RGBA32{static_cast<std::uint8_t>(red), static_cast<std::uint8_t>(green), static_cast<std::uint8_t>(blue),
                  ALPHA_OPAQUE};
}

static TilesOutputPalette parseTilesPngPaletteMode(const PorytilesContext &ctx, const std::string &optionName,
                                                   const char *optarg) {
    std::string optargString{optarg};
    if (optargString == "true-color") {
        return TilesOutputPalette::TRUE_COLOR;
    }
    if (optargString == "greyscale") {
        return TilesOutputPalette::GREYSCALE;
    }
    const auto msg =
        fmt::format("invalid argument '{}' for option '{}'", ctx.diag->Bold(optargString), ctx.diag->Bold(optionName));
    ctx.diag->Report(kFatalGeneric, msg);
    throw PorytilesException{msg};
}

static TargetBaseGame parseTargetBaseGame(const PorytilesContext &ctx, const std::string &optionName,
                                          const char *optarg) {
    std::string optargString{optarg};
    if (optargString == "pokeemerald") {
        return TargetBaseGame::EMERALD;
    }
    if (optargString == "pokefirered") {
        return TargetBaseGame::FIRERED;
    }
    if (optargString == "pokeruby") {
        return TargetBaseGame::RUBY;
    }
    const auto msg =
        fmt::format("invalid argument '{}' for option '{}'", ctx.diag->Bold(optargString), ctx.diag->Bold(optionName));
    ctx.diag->Report(kFatalGeneric, msg);
    throw PorytilesException{msg};
}

static AssignAlgorithm parseAssignAlgorithm(const PorytilesContext &ctx, const std::string &optionName,
                                            const char *optarg) {
    const std::string optargString{optarg};
    if (optargString == assignAlgorithmString(AssignAlgorithm::DFS)) {
        return AssignAlgorithm::DFS;
    }
    if (optargString == assignAlgorithmString(AssignAlgorithm::BFS)) {
        return AssignAlgorithm::BFS;
    }
    const auto msg =
        fmt::format("invalid argument `{}' for option `{}'", ctx.diag->Bold(optargString), ctx.diag->Bold(optionName));
    ctx.diag->Report(kFatalGeneric, msg);
    throw PorytilesException{msg};
}

constexpr std::vector<std::string> GLOBAL_SHORTS = {};
static void parseGlobalOptions(PorytilesContext &ctx, int argc, char *const *argv) {
    std::ostringstream implodedShorts;
    std::copy(GLOBAL_SHORTS.begin(), GLOBAL_SHORTS.end(), std::ostream_iterator<std::string>(implodedShorts, ""));
    // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
    std::string shortOptions = "+" + implodedShorts.str();
    static struct option longOptions[] = {{HELP.c_str(), no_argument, nullptr, HELP_VAL},
                                          {HELP_SHORT.c_str(), no_argument, nullptr, HELP_VAL},
                                          {VERBOSE.c_str(), no_argument, nullptr, VERBOSE_VAL},
                                          {VERBOSE_SHORT.c_str(), no_argument, nullptr, VERBOSE_VAL},
                                          {VERSION.c_str(), no_argument, nullptr, VERSION_VAL},
                                          {VERSION_SHORT.c_str(), no_argument, nullptr, VERSION_VAL},
                                          {nullptr, no_argument, nullptr, 0}};

    while (true) {
        const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

        if (opt == -1)
            break;

        switch (opt) {
        case VERBOSE_VAL:
            ctx.verbose = true;
            break;
        case VERSION_VAL:
            fmt::println("{} {} {}", PORYTILES_EXECUTABLE, PORYTILES_BUILD_VERSION, PORYTILES_BUILD_DATE);
            exit(0);

            // Help message upon '-h/--help' goes to stdout
        case HELP_VAL:
            fmt::println("{}", GLOBAL_HELP);
            exit(0);
            // Help message on invalid or unknown options goes to stderr and gives error code
        case '?':
        default:
            fmt::println(stderr, "Try `{} --help' for usage information.", PORYTILES_EXECUTABLE);
            exit(2);
        }
    }
}

static void parseSubcommand(PorytilesContext &ctx, int argc, char *const *argv) {
    if ((argc - optind) == 0) {
        const auto msg = "missing required subcommand, try 'porytiles --help' for usage information";
        ctx.diag->Report(kFatalGeneric, msg);
        throw PorytilesException{msg};
    }

    std::string subcommand = argv[optind++];
    if (subcommand == DECOMPILE_PRIMARY_COMMAND) {
        ctx.subcommand = Subcommand::DECOMPILE_PRIMARY;
    } else if (subcommand == DECOMPILE_SECONDARY_COMMAND) {
        ctx.subcommand = Subcommand::DECOMPILE_SECONDARY;
    } else if (subcommand == COMPILE_PRIMARY_COMMAND) {
        ctx.subcommand = Subcommand::COMPILE_PRIMARY;
    } else if (subcommand == COMPILE_SECONDARY_COMMAND) {
        ctx.subcommand = Subcommand::COMPILE_SECONDARY;
    } else {
        const auto msg =
            fmt::format("unrecognized subcommand '{}', try 'porytiles --help' for usage information", subcommand);
        ctx.diag->Report(kFatalGeneric, msg);
        throw PorytilesException{msg};
    }
}

static void validateSubcommandContext(PorytilesContext &ctx, std::string option) {
    if (!supportedSubcommands.contains(option)) {
        Panic(fmt::format("'supportedSubcommands' did not contain mapping for option `{}'", option));
    }
    if (!supportedSubcommands.at(option).contains(ctx.subcommand)) {
        pt_fatal_err("unrecognized option '{}' for subcommand '{}'", option, subcommandString(ctx.subcommand));
        pt_println(stderr, "Try '{} --help' for usage information.", subcommandString(ctx.subcommand));
        throw PorytilesException{
            fmt::format("unrecognized option '{}' for subcommand '{}'", option, subcommandString(ctx.subcommand))};
    }
}

const std::vector<std::string> COMPILE_SHORTS = {};
/*
 * FIXME : the warning parsing system here is a dumpster fire
 */
static void parseSubcommandOptions(PorytilesContext &ctx, int argc, char *const *argv) {
    std::ostringstream implodedShorts;
    std::copy(COMPILE_SHORTS.begin(), COMPILE_SHORTS.end(), std::ostream_iterator<std::string>(implodedShorts, ""));
    // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
    std::string shortOptions = "+" + implodedShorts.str();
    struct option longOptions[] = {
        // Driver options
        {OUTPUT.c_str(), required_argument, nullptr, OUTPUT_VAL},
        {OUTPUT_SHORT.c_str(), required_argument, nullptr, OUTPUT_VAL},
        {TILES_OUTPUT_PAL.c_str(), required_argument, nullptr, TILES_OUTPUT_PAL_VAL},
        {NORMALIZE_TRANSPARENCY.c_str(), optional_argument, nullptr, NORMALIZE_TRANSPARENCY_VAL},
        {PRESERVE_TRANSPARENCY.c_str(), no_argument, nullptr, PRESERVE_TRANSPARENCY_VAL},
        {DISABLE_METATILE_GENERATION.c_str(), no_argument, nullptr, DISABLE_METATILE_GENERATION_VAL},
        {DISABLE_ATTRIBUTE_GENERATION.c_str(), no_argument, nullptr, DISABLE_ATTRIBUTE_GENERATION_VAL},

        // Tileset generation options
        {TARGET_BASE_GAME.c_str(), required_argument, nullptr, TARGET_BASE_GAME_VAL},
        {DUAL_LAYER.c_str(), no_argument, nullptr, DUAL_LAYER_VAL},
        {TRANSPARENCY_COLOR.c_str(), required_argument, nullptr, TRANSPARENCY_COLOR_VAL},
        {DEFAULT_BEHAVIOR.c_str(), required_argument, nullptr, DEFAULT_BEHAVIOR_VAL},
        {DEFAULT_ENCOUNTER_TYPE.c_str(), required_argument, nullptr, DEFAULT_ENCOUNTER_TYPE_VAL},
        {DEFAULT_TERRAIN_TYPE.c_str(), required_argument, nullptr, DEFAULT_TERRAIN_TYPE_VAL},

        // Color assignment config options
        {EXPLORE_CUTOFF.c_str(), required_argument, nullptr, EXPLORE_CUTOFF_VAL},
        {ASSIGN_ALGO.c_str(), required_argument, nullptr, ASSIGN_ALGO_VAL},
        {BEST_BRANCHES.c_str(), required_argument, nullptr, BEST_BRANCHES_VAL},
        {PRIMARY_EXPLORE_CUTOFF.c_str(), required_argument, nullptr, PRIMARY_EXPLORE_CUTOFF_VAL},
        {PRIMARY_ASSIGN_ALGO.c_str(), required_argument, nullptr, PRIMARY_ASSIGN_ALGO_VAL},
        {PRIMARY_BEST_BRANCHES.c_str(), required_argument, nullptr, PRIMARY_BEST_BRANCHES_VAL},

        // Fieldmap override options
        {TILES_PRIMARY_OVERRIDE.c_str(), required_argument, nullptr, TILES_PRIMARY_OVERRIDE_VAL},
        {TILES_TOTAL_OVERRIDE.c_str(), required_argument, nullptr, TILES_TOTAL_OVERRIDE_VAL},
        {METATILES_PRIMARY_OVERRIDE.c_str(), required_argument, nullptr, METATILES_PRIMARY_OVERRIDE_VAL},
        {METATILES_TOTAL_OVERRIDE.c_str(), required_argument, nullptr, METATILES_TOTAL_OVERRIDE_VAL},
        {PALS_PRIMARY_OVERRIDE.c_str(), required_argument, nullptr, PALS_PRIMARY_OVERRIDE_VAL},
        {PALS_TOTAL_OVERRIDE.c_str(), required_argument, nullptr, PALS_TOTAL_OVERRIDE_VAL},

        // Warning and error options
        {WALL.c_str(), no_argument, nullptr, WALL_VAL},
        {WNONE.c_str(), no_argument, nullptr, WNONE_VAL},
        {WNONE_SHORT.c_str(), no_argument, nullptr, WNONE_VAL},
        {WERROR.c_str(), optional_argument, nullptr, WERROR_VAL},
        {WNO_ERROR.c_str(), required_argument, nullptr, WNO_ERROR_VAL},

        // Compilation warnings
        {WCOLOR_PRECISION_LOSS.c_str(), no_argument, nullptr, WCOLOR_PRECISION_LOSS_VAL},
        {WNO_COLOR_PRECISION_LOSS.c_str(), no_argument, nullptr, WNO_COLOR_PRECISION_LOSS_VAL},

        {WKEY_FRAME_NO_MATCHING_TILE.c_str(), no_argument, nullptr, WKEY_FRAME_NO_MATCHING_TILE_VAL},
        {WNO_KEY_FRAME_NO_MATCHING_TILE.c_str(), no_argument, nullptr, WNO_KEY_FRAME_NO_MATCHING_TILE_VAL},

        {WKEY_FRAME_MISSING_COLORS.c_str(), no_argument, nullptr, WKEY_FRAME_MISSING_COLORS_VAL},
        {WNO_KEY_FRAME_MISSING_COLORS.c_str(), no_argument, nullptr, WNO_KEY_FRAME_MISSING_COLORS_VAL},

        {WATTRIBUTE_FORMAT_MISMATCH.c_str(), no_argument, nullptr, WATTRIBUTE_FORMAT_MISMATCH_VAL},
        {WNO_ATTRIBUTE_FORMAT_MISMATCH.c_str(), no_argument, nullptr, WNO_ATTRIBUTE_FORMAT_MISMATCH_VAL},

        {WMISSING_ATTRIBUTES_CSV.c_str(), no_argument, nullptr, WMISSING_ATTRIBUTES_CSV_VAL},
        {WNO_MISSING_ATTRIBUTES_CSV.c_str(), no_argument, nullptr, WNO_MISSING_ATTRIBUTES_CSV_VAL},

        {WUNUSED_ATTRIBUTE.c_str(), no_argument, nullptr, WUNUSED_ATTRIBUTE_VAL},
        {WNO_UNUSED_ATTRIBUTE.c_str(), no_argument, nullptr, WNO_UNUSED_ATTRIBUTE_VAL},

        {WTRANSPARENCY_COLLAPSE.c_str(), no_argument, nullptr, WTRANSPARENCY_COLLAPSE_VAL},
        {WNO_TRANSPARENCY_COLLAPSE.c_str(), no_argument, nullptr, WNO_TRANSPARENCY_COLLAPSE_VAL},

        {WUNUSED_MANUAL_PAL_COLOR.c_str(), no_argument, nullptr, WUNUSED_MANUAL_PAL_COLOR_VAL},
        {WNO_UNUSED_MANUAL_PAL_COLOR.c_str(), no_argument, nullptr, WNO_UNUSED_MANUAL_PAL_COLOR_VAL},

        // Decompilation warnings
        {WTILE_INDEX_OUT_OF_RANGE.c_str(), no_argument, nullptr, WTILE_INDEX_OUT_OF_RANGE_VAL},
        {WNO_TILE_INDEX_OUT_OF_RANGE.c_str(), no_argument, nullptr, WNO_TILE_INDEX_OUT_OF_RANGE_VAL},

        {WPALETTE_INDEX_OUT_OF_RANGE.c_str(), no_argument, nullptr, WPALETTE_INDEX_OUT_OF_RANGE_VAL},
        {WNO_PALETTE_INDEX_OUT_OF_RANGE.c_str(), no_argument, nullptr, WNO_PALETTE_INDEX_OUT_OF_RANGE_VAL},

        // Help
        {HELP.c_str(), no_argument, nullptr, HELP_VAL},
        {HELP_SHORT.c_str(), no_argument, nullptr, HELP_VAL},

        {nullptr, no_argument, nullptr, 0}};

    /*
     * Fieldmap specific variables. Like warnings above, we must wait until after all options are processed before we
     * start applying the fieldmap config. We want specific fieldmap overrides to take precedence over the general
     * target base game, no matter where in the command line things were specified.
     */
    bool tilesPrimaryOverridden = false;
    std::size_t tilesPrimaryOverride = 0;
    bool tilesTotalOverridden = false;
    std::size_t tilesTotalOverride = 0;
    bool metatilesPrimaryOverridden = false;
    std::size_t metatilesPrimaryOverride = 0;
    bool metatilesTotalOverridden = false;
    std::size_t metatilesTotalOverride = 0;
    bool palettesPrimaryOverridden = false;
    std::size_t palettesPrimaryOverride = 0;
    bool palettesTotalOverridden = false;
    std::size_t palettesTotalOverride = 0;

    std::size_t exploreCutoff;

    while (true) {
        const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

        if (opt == -1)
            break;

        switch (opt) {

        // Driver options
        case OUTPUT_VAL:
            validateSubcommandContext(ctx, OUTPUT);
            ctx.output.path = optarg;
            break;
        case TILES_OUTPUT_PAL_VAL:
            validateSubcommandContext(ctx, TILES_OUTPUT_PAL);
            ctx.output.paletteMode = parseTilesPngPaletteMode(ctx, TILES_OUTPUT_PAL, optarg);
            break;
        case DISABLE_METATILE_GENERATION_VAL:
            validateSubcommandContext(ctx, DISABLE_METATILE_GENERATION);
            ctx.output.disableMetatileGeneration = true;
            break;
        case DISABLE_ATTRIBUTE_GENERATION_VAL:
            validateSubcommandContext(ctx, DISABLE_ATTRIBUTE_GENERATION);
            ctx.output.disableAttributeGeneration = true;
            break;

        // Tileset (de)compilation options
        case TARGET_BASE_GAME_VAL:
            validateSubcommandContext(ctx, TARGET_BASE_GAME);
            ctx.targetBaseGame = parseTargetBaseGame(ctx, TARGET_BASE_GAME, optarg);
            break;
        case DUAL_LAYER_VAL:
            validateSubcommandContext(ctx, DUAL_LAYER);
            ctx.compilerConfig.tripleLayer = false;
            break;
        case TRANSPARENCY_COLOR_VAL:
            validateSubcommandContext(ctx, TRANSPARENCY_COLOR);
            ctx.compilerConfig.transparencyColor = parseRgbColor(ctx, TRANSPARENCY_COLOR, optarg);
            break;
        case DEFAULT_BEHAVIOR_VAL:
            validateSubcommandContext(ctx, DEFAULT_BEHAVIOR);
            ctx.compilerConfig.defaultBehavior = std::string{optarg};
            break;
        case DEFAULT_ENCOUNTER_TYPE_VAL:
            validateSubcommandContext(ctx, DEFAULT_ENCOUNTER_TYPE);
            ctx.compilerConfig.defaultEncounterType = std::string{optarg};
            break;
        case DEFAULT_TERRAIN_TYPE_VAL:
            validateSubcommandContext(ctx, DEFAULT_TERRAIN_TYPE);
            ctx.compilerConfig.defaultTerrainType = std::string{optarg};
            break;
        case NORMALIZE_TRANSPARENCY_VAL:
            validateSubcommandContext(ctx, NORMALIZE_TRANSPARENCY);
            ctx.decompilerConfig.normalizeTransparency = true;
            if (optarg != nullptr) {
                ctx.decompilerConfig.normalizeTransparencyColor = parseRgbColor(ctx, NORMALIZE_TRANSPARENCY, optarg);
            } else {
                // TODO : remove this deprecation warning at some point in the future
                pt_warn("the no-arg version of `normalize-transparency' has been deprecated");
                pt_println(
                    stderr,
                    "         It is now the default Porytiles behavior, so you no longer need to specify this option.");
                pt_println(stderr, "         In a future version, it will be removed.");
            }
            break;
        case PRESERVE_TRANSPARENCY_VAL:
            validateSubcommandContext(ctx, PRESERVE_TRANSPARENCY);
            ctx.decompilerConfig.normalizeTransparency = false;
            break;

        // Color assignment config options
        case EXPLORE_CUTOFF_VAL:
            validateSubcommandContext(ctx, EXPLORE_CUTOFF);
            ctx.compilerConfig.providedAssignOverride = true;
            exploreCutoff = parseIntegralOption<std::size_t>(ctx, EXPLORE_CUTOFF, optarg);
            if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
                ctx.compilerConfig.primaryExploredNodeCutoff = exploreCutoff;
                if (ctx.compilerConfig.primaryExploredNodeCutoff > EXPLORATION_MAX_CUTOFF) {
                    const auto msg =
                        fmt::format("option '{}' argument cannot be > 100", ctx.diag->Bold(EXPLORE_CUTOFF));
                    ctx.diag->Report(kFatalGeneric, msg);
                    throw PorytilesException{msg};
                }
            } else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
                ctx.compilerConfig.secondaryExploredNodeCutoff = exploreCutoff;
                if (ctx.compilerConfig.secondaryExploredNodeCutoff > EXPLORATION_MAX_CUTOFF) {
                    const auto msg =
                        fmt::format("option '{}' argument cannot be > 100", ctx.diag->Bold(EXPLORE_CUTOFF));
                    ctx.diag->Report(kFatalGeneric, msg);
                    throw PorytilesException{msg};
                }
            }
            break;
        case ASSIGN_ALGO_VAL:
            validateSubcommandContext(ctx, ASSIGN_ALGO);
            ctx.compilerConfig.providedAssignOverride = true;
            if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
                ctx.compilerConfig.primaryAssignAlgorithm = parseAssignAlgorithm(ctx, ASSIGN_ALGO, optarg);
            } else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
                ctx.compilerConfig.secondaryAssignAlgorithm = parseAssignAlgorithm(ctx, ASSIGN_ALGO, optarg);
            }
            break;
        case BEST_BRANCHES_VAL:
            validateSubcommandContext(ctx, BEST_BRANCHES);
            ctx.compilerConfig.providedAssignOverride = true;
            if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
                if (std::string{optarg} == SMART_PRUNE) {
                    ctx.compilerConfig.primarySmartPrune = true;
                } else {
                    ctx.compilerConfig.primaryBestBranches =
                        parseIntegralOption<std::size_t>(ctx, BEST_BRANCHES, optarg);
                    if (ctx.compilerConfig.primaryBestBranches == 0) {
                        const auto msg = fmt::format("option '{}' argument cannot be 0", ctx.diag->Bold(BEST_BRANCHES));
                        ctx.diag->Report(kFatalGeneric, msg);
                        throw PorytilesException{msg};
                    }
                }
            } else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
                if (std::string{optarg} == SMART_PRUNE) {
                    ctx.compilerConfig.secondarySmartPrune = true;
                } else {
                    ctx.compilerConfig.secondaryBestBranches =
                        parseIntegralOption<std::size_t>(ctx, BEST_BRANCHES, optarg);
                    if (ctx.compilerConfig.secondaryBestBranches == 0) {
                        const auto msg = fmt::format("option '{}' argument cannot be 0", ctx.diag->Bold(BEST_BRANCHES));
                        ctx.diag->Report(kFatalGeneric, msg);
                        throw PorytilesException{msg};
                    }
                }
            }
            break;
        case PRIMARY_EXPLORE_CUTOFF_VAL:
            validateSubcommandContext(ctx, PRIMARY_EXPLORE_CUTOFF);
            ctx.compilerConfig.providedPrimaryAssignOverride = true;
            exploreCutoff = parseIntegralOption<std::size_t>(ctx, PRIMARY_EXPLORE_CUTOFF, optarg);
            if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
                ctx.compilerConfig.primaryExploredNodeCutoff = exploreCutoff;
                if (ctx.compilerConfig.primaryExploredNodeCutoff > EXPLORATION_MAX_CUTOFF) {
                    const auto msg =
                        fmt::format("option '{}' argument cannot be > 100", ctx.diag->Bold(PRIMARY_EXPLORE_CUTOFF));
                    ctx.diag->Report(kFatalGeneric, msg);
                    throw PorytilesException{msg};
                }
            }
            break;
        case PRIMARY_ASSIGN_ALGO_VAL:
            validateSubcommandContext(ctx, PRIMARY_ASSIGN_ALGO);
            ctx.compilerConfig.providedPrimaryAssignOverride = true;
            if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
                ctx.compilerConfig.primaryAssignAlgorithm = parseAssignAlgorithm(ctx, PRIMARY_ASSIGN_ALGO, optarg);
            }
            break;
        case PRIMARY_BEST_BRANCHES_VAL:
            validateSubcommandContext(ctx, PRIMARY_BEST_BRANCHES);
            ctx.compilerConfig.providedPrimaryAssignOverride = true;
            if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
                if (std::string{optarg} == "smart") {
                    ctx.compilerConfig.primarySmartPrune = true;
                } else {
                    ctx.compilerConfig.primaryBestBranches =
                        parseIntegralOption<std::size_t>(ctx, PRIMARY_BEST_BRANCHES, optarg);
                    if (ctx.compilerConfig.primaryBestBranches == 0) {
                        const auto msg =
                            fmt::format("option '{}' argument cannot be 0", ctx.diag->Bold(PRIMARY_BEST_BRANCHES));
                        ctx.diag->Report(kFatalGeneric, msg);
                        throw PorytilesException{msg};
                    }
                }
            }
            break;

        // Fieldmap override options
        case TILES_PRIMARY_OVERRIDE_VAL:
            validateSubcommandContext(ctx, TILES_PRIMARY_OVERRIDE);
            tilesPrimaryOverridden = true;
            tilesPrimaryOverride = parseIntegralOption<std::size_t>(ctx, TILES_PRIMARY_OVERRIDE, optarg);
            break;
        case TILES_TOTAL_OVERRIDE_VAL:
            validateSubcommandContext(ctx, TILES_TOTAL_OVERRIDE);
            tilesTotalOverridden = true;
            tilesTotalOverride = parseIntegralOption<std::size_t>(ctx, TILES_TOTAL_OVERRIDE, optarg);
            break;
        case METATILES_PRIMARY_OVERRIDE_VAL:
            validateSubcommandContext(ctx, METATILES_PRIMARY_OVERRIDE);
            metatilesPrimaryOverridden = true;
            metatilesPrimaryOverride = parseIntegralOption<std::size_t>(ctx, METATILES_PRIMARY_OVERRIDE, optarg);
            break;
        case METATILES_TOTAL_OVERRIDE_VAL:
            validateSubcommandContext(ctx, METATILES_TOTAL_OVERRIDE);
            metatilesTotalOverridden = true;
            metatilesTotalOverride = parseIntegralOption<std::size_t>(ctx, METATILES_TOTAL_OVERRIDE, optarg);
            break;
        case PALS_PRIMARY_OVERRIDE_VAL:
            validateSubcommandContext(ctx, PALS_PRIMARY_OVERRIDE);
            palettesPrimaryOverridden = true;
            palettesPrimaryOverride = parseIntegralOption<std::size_t>(ctx, PALS_PRIMARY_OVERRIDE, optarg);
            break;
        case PALS_TOTAL_OVERRIDE_VAL:
            validateSubcommandContext(ctx, PALS_TOTAL_OVERRIDE);
            palettesTotalOverridden = true;
            palettesTotalOverride = parseIntegralOption<std::size_t>(ctx, PALS_TOTAL_OVERRIDE, optarg);
            break;

        // Warning and error options
        case WALL_VAL:
            validateSubcommandContext(ctx, WALL);
            ctx.diag->EnableAllWarnings();
            break;
        case WNONE_VAL:
            validateSubcommandContext(ctx, WNONE);
            ctx.diag->DisableAllWarnings();
            break;
        case WERROR_VAL:
            validateSubcommandContext(ctx, WERROR);
            if (optarg == nullptr) {
                ctx.diag->UpgradeEnabledWarningsToErr();
            } else {
                // Compilation warnings
                if (strcmp(optarg, kWarnColorPrecisionLoss) == 0) {
                    ctx.diag->EnableAtLevel(kWarnColorPrecisionLoss, DiagLevel::Error);
                } else if (strcmp(optarg, kWarnKeyFrameNoMatchingTile) == 0) {
                    ctx.diag->EnableAtLevel(kWarnKeyFrameNoMatchingTile, DiagLevel::Error);
                } else if (strcmp(optarg, kWarnKeyFrameMissingColors) == 0) {
                    ctx.diag->EnableAtLevel(kWarnKeyFrameMissingColors, DiagLevel::Error);
                } else if (strcmp(optarg, kWarnAttributeFormatMismatch) == 0) {
                    ctx.diag->EnableAtLevel(kWarnAttributeFormatMismatch, DiagLevel::Error);
                } else if (strcmp(optarg, kWarnMissingAttributesCsv) == 0) {
                    ctx.diag->EnableAtLevel(kWarnMissingAttributesCsv, DiagLevel::Error);
                } else if (strcmp(optarg, kWarnUnusedAttribute) == 0) {
                    ctx.diag->EnableAtLevel(kWarnUnusedAttribute, DiagLevel::Error);
                } else if (strcmp(optarg, kWarnTransparencyCollapse) == 0) {
                    ctx.diag->EnableAtLevel(kWarnTransparencyCollapse, DiagLevel::Error);
                } else if (strcmp(optarg, kWarnUnusedManualPalColor) == 0) {
                    ctx.diag->EnableAtLevel(kWarnUnusedManualPalColor, DiagLevel::Error);
                }
                // Decompilation warnings
                else if (strcmp(optarg, kWarnTileIndexOutOfRange) == 0) {
                    ctx.diag->EnableAtLevel(kWarnTileIndexOutOfRange, DiagLevel::Error);
                } else if (strcmp(optarg, kWarnPaletteIndexOutOfRange) == 0) {
                    ctx.diag->EnableAtLevel(kWarnPaletteIndexOutOfRange, DiagLevel::Error);
                } else {
                    const auto msg = fmt::format("invalid argument '{}' for option '{}'",
                                                 ctx.diag->Bold(std::string{optarg}), ctx.diag->Bold(WERROR));
                    ctx.diag->Report(kFatalGeneric, msg);
                    throw PorytilesException{msg};
                }
            }
            break;
        case WNO_ERROR_VAL:
            validateSubcommandContext(ctx, WNO_ERROR);
            // Compilation warnings
            if (strcmp(optarg, kWarnColorPrecisionLoss) == 0) {
                ctx.diag->DisableAtLevel(kWarnColorPrecisionLoss, DiagLevel::Error);
            } else if (strcmp(optarg, kWarnKeyFrameNoMatchingTile) == 0) {
                ctx.diag->DisableAtLevel(kWarnKeyFrameNoMatchingTile, DiagLevel::Error);
            } else if (strcmp(optarg, kWarnKeyFrameMissingColors) == 0) {
                ctx.diag->DisableAtLevel(kWarnKeyFrameMissingColors, DiagLevel::Error);
            } else if (strcmp(optarg, kWarnAttributeFormatMismatch) == 0) {
                ctx.diag->DisableAtLevel(kWarnAttributeFormatMismatch, DiagLevel::Error);
            } else if (strcmp(optarg, kWarnMissingAttributesCsv) == 0) {
                ctx.diag->DisableAtLevel(kWarnMissingAttributesCsv, DiagLevel::Error);
            } else if (strcmp(optarg, kWarnUnusedAttribute) == 0) {
                ctx.diag->DisableAtLevel(kWarnUnusedAttribute, DiagLevel::Error);
            } else if (strcmp(optarg, kWarnTransparencyCollapse) == 0) {
                ctx.diag->DisableAtLevel(kWarnTransparencyCollapse, DiagLevel::Error);
            } else if (strcmp(optarg, kWarnUnusedManualPalColor) == 0) {
                ctx.diag->DisableAtLevel(kWarnUnusedManualPalColor, DiagLevel::Error);
            }
            // Decompilation warnings
            else if (strcmp(optarg, kWarnTileIndexOutOfRange) == 0) {
                ctx.diag->DisableAtLevel(kWarnTileIndexOutOfRange, DiagLevel::Error);
            } else if (strcmp(optarg, kWarnPaletteIndexOutOfRange) == 0) {
                ctx.diag->DisableAtLevel(kWarnPaletteIndexOutOfRange, DiagLevel::Error);
            } else {
                const auto msg = fmt::format("invalid argument '{}' for option '{}'",
                                             ctx.diag->Bold(std::string{optarg}), ctx.diag->Bold(WERROR));
                ctx.diag->Report(kFatalGeneric, msg);
                throw PorytilesException{msg};
            }
            break;

        // Compilation warnings
        case WCOLOR_PRECISION_LOSS_VAL:
            validateSubcommandContext(ctx, WCOLOR_PRECISION_LOSS);
            ctx.diag->EnableAtLevel(kWarnColorPrecisionLoss, DiagLevel::Warning);
            break;
        case WNO_COLOR_PRECISION_LOSS_VAL:
            validateSubcommandContext(ctx, WNO_COLOR_PRECISION_LOSS);
            ctx.diag->DisableAtLevel(kWarnColorPrecisionLoss, DiagLevel::Warning);
            break;
        case WKEY_FRAME_NO_MATCHING_TILE_VAL:
            validateSubcommandContext(ctx, WKEY_FRAME_NO_MATCHING_TILE);
            ctx.diag->EnableAtLevel(kWarnKeyFrameNoMatchingTile, DiagLevel::Warning);
            break;
        case WNO_KEY_FRAME_NO_MATCHING_TILE_VAL:
            validateSubcommandContext(ctx, WNO_KEY_FRAME_NO_MATCHING_TILE);
            ctx.diag->DisableAtLevel(kWarnKeyFrameNoMatchingTile, DiagLevel::Warning);
            break;
        case WKEY_FRAME_MISSING_COLORS_VAL:
            validateSubcommandContext(ctx, WKEY_FRAME_MISSING_COLORS);
            ctx.diag->EnableAtLevel(kWarnKeyFrameMissingColors, DiagLevel::Warning);
            break;
        case WNO_KEY_FRAME_MISSING_COLORS_VAL:
            validateSubcommandContext(ctx, WNO_KEY_FRAME_MISSING_COLORS);
            ctx.diag->DisableAtLevel(kWarnKeyFrameMissingColors, DiagLevel::Warning);
            break;
        case WATTRIBUTE_FORMAT_MISMATCH_VAL:
            validateSubcommandContext(ctx, WATTRIBUTE_FORMAT_MISMATCH);
            ctx.diag->EnableAtLevel(kWarnAttributeFormatMismatch, DiagLevel::Warning);
            break;
        case WNO_ATTRIBUTE_FORMAT_MISMATCH_VAL:
            validateSubcommandContext(ctx, WNO_ATTRIBUTE_FORMAT_MISMATCH);
            ctx.diag->DisableAtLevel(kWarnAttributeFormatMismatch, DiagLevel::Warning);
            break;
        case WMISSING_ATTRIBUTES_CSV_VAL:
            validateSubcommandContext(ctx, WMISSING_ATTRIBUTES_CSV);
            ctx.diag->EnableAtLevel(kWarnMissingAttributesCsv, DiagLevel::Warning);
            break;
        case WNO_MISSING_ATTRIBUTES_CSV_VAL:
            validateSubcommandContext(ctx, WNO_MISSING_ATTRIBUTES_CSV);
            ctx.diag->DisableAtLevel(kWarnMissingAttributesCsv, DiagLevel::Warning);
            break;
        case WUNUSED_ATTRIBUTE_VAL:
            validateSubcommandContext(ctx, WUNUSED_ATTRIBUTE);
            ctx.diag->EnableAtLevel(kWarnUnusedAttribute, DiagLevel::Warning);
            break;
        case WNO_UNUSED_ATTRIBUTE_VAL:
            validateSubcommandContext(ctx, WNO_UNUSED_ATTRIBUTE);
            ctx.diag->DisableAtLevel(kWarnUnusedAttribute, DiagLevel::Warning);
            break;
        case WTRANSPARENCY_COLLAPSE_VAL:
            validateSubcommandContext(ctx, WTRANSPARENCY_COLLAPSE);
            ctx.diag->EnableAtLevel(kWarnTransparencyCollapse, DiagLevel::Warning);
            break;
        case WNO_TRANSPARENCY_COLLAPSE_VAL:
            validateSubcommandContext(ctx, WNO_TRANSPARENCY_COLLAPSE);
            ctx.diag->DisableAtLevel(kWarnTransparencyCollapse, DiagLevel::Warning);
            break;
        case WUNUSED_MANUAL_PAL_COLOR_VAL:
            validateSubcommandContext(ctx, WUNUSED_MANUAL_PAL_COLOR);
            ctx.diag->EnableAtLevel(kWarnUnusedManualPalColor, DiagLevel::Warning);
            break;
        case WNO_UNUSED_MANUAL_PAL_COLOR_VAL:
            validateSubcommandContext(ctx, WNO_UNUSED_MANUAL_PAL_COLOR);
            ctx.diag->DisableAtLevel(kWarnUnusedManualPalColor, DiagLevel::Warning);
            break;
        // Decompilation warnings
        case WTILE_INDEX_OUT_OF_RANGE_VAL:
            validateSubcommandContext(ctx, WTILE_INDEX_OUT_OF_RANGE);
            ctx.diag->EnableAtLevel(kWarnTileIndexOutOfRange, DiagLevel::Warning);
            break;
        case WNO_TILE_INDEX_OUT_OF_RANGE_VAL:
            validateSubcommandContext(ctx, WNO_TILE_INDEX_OUT_OF_RANGE);
            ctx.diag->DisableAtLevel(kWarnTileIndexOutOfRange, DiagLevel::Warning);
            break;
        case WPALETTE_INDEX_OUT_OF_RANGE_VAL:
            validateSubcommandContext(ctx, WPALETTE_INDEX_OUT_OF_RANGE);
            ctx.diag->EnableAtLevel(kWarnPaletteIndexOutOfRange, DiagLevel::Warning);
            break;
        case WNO_PALETTE_INDEX_OUT_OF_RANGE_VAL:
            validateSubcommandContext(ctx, WNO_PALETTE_INDEX_OUT_OF_RANGE);
            ctx.diag->DisableAtLevel(kWarnPaletteIndexOutOfRange, DiagLevel::Warning);
            break;

        // Help message upon '-h/--help' goes to stdout
        case HELP_VAL:
            validateSubcommandContext(ctx, HELP);
            if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
                fmt::println("{}", COMPILE_PRIMARY_HELP);
            } else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
                fmt::println("{}", COMPILE_SECONDARY_HELP);
            } else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY) {
                fmt::println("{}", DECOMPILE_PRIMARY_HELP);
            } else if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
                fmt::println("{}", DECOMPILE_SECONDARY_HELP);
            } else {
                Panic(fmt::format("cli_parser::parseSubcommandOptions unknown subcommand: {}",
                                  static_cast<int>(ctx.subcommand)));
            }
            exit(0);
        // Help message on invalid or unknown options goes to stderr and gives error code
        case '?':
        default:
            // TODO : figure out how to use fatalerror_unrecognizedOption here
            fmt::println(stderr, "Try '{} {} --help' for usage information.", PORYTILES_EXECUTABLE,
                         subcommandString(ctx.subcommand));
            exit(2);
        }
    }

    /*
     * Die immediately if arguments are invalid, otherwise pack them into the context variable
     */
    if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
        if ((argc - optind) != 2) {
            const auto msg = "must specify INPUT-PATH, BEHAVIORS-HEADER args, see 'porytiles compile-primary --help'";
            ctx.diag->Report(kFatalGeneric, msg);
            throw PorytilesException{msg};
        }
    } else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        if ((argc - optind) != 3) {
            const auto msg = "must specify INPUT-PATH, PRIMARY-INPUT-PATH, BEHAVIORS-HEADER args, see 'porytiles "
                             "compile-secondary --help'";
            ctx.diag->Report(kFatalGeneric, msg);
            throw PorytilesException{msg};
        }
    } else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY) {
        if ((argc - optind) != 2) {
            const auto msg = "must specify INPUT-PATH, BEHAVIORS-HEADER args, see 'porytiles decompile-primary --help'";
            ctx.diag->Report(kFatalGeneric, msg);
            throw PorytilesException{msg};
        }
    } else if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
        if ((argc - optind) != 3) {
            const auto msg = "must specify INPUT-PATH, PRIMARY-INPUT-PATH, BEHAVIORS-HEADER args, see 'porytiles "
                             "decompile-secondary --help'";
            ctx.diag->Report(kFatalGeneric, msg);
            throw PorytilesException{msg};
        }
    } else {
        Panic(
            fmt::format("cli_parser::parseSubcommandOptions unknown subcommand: {}", static_cast<int>(ctx.subcommand)));
    }

    if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        ctx.compilerSrcPaths.secondarySourcePath = argv[optind++];
    } else if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
        ctx.decompilerSrcPaths.secondarySourcePath = argv[optind++];
    }

    if (ctx.subcommand == Subcommand::COMPILE_PRIMARY || ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        ctx.compilerSrcPaths.primarySourcePath = argv[optind++];
        ctx.compilerSrcPaths.metatileBehaviors = argv[optind++];
    } else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY || ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
        ctx.decompilerSrcPaths.primarySourcePath = argv[optind++];
        ctx.decompilerSrcPaths.metatileBehaviors = argv[optind++];
    } else {
        Panic(
            fmt::format("cli_parser::parseSubcommandOptions unknown subcommand: {}", static_cast<int>(ctx.subcommand)));
    }

    /*
     * Apply and validate the fieldmap configuration parameters
     */
    if (ctx.targetBaseGame == TargetBaseGame::EMERALD) {
        ctx.fieldmapConfig = FieldmapConfig::pokeemeraldDefaults();
    } else if (ctx.targetBaseGame == TargetBaseGame::FIRERED) {
        ctx.fieldmapConfig = FieldmapConfig::pokefireredDefaults();
    } else if (ctx.targetBaseGame == TargetBaseGame::RUBY) {
        ctx.fieldmapConfig = FieldmapConfig::pokerubyDefaults();
    }
    if (tilesPrimaryOverridden) {
        ctx.fieldmapConfig.numTilesInPrimary = tilesPrimaryOverride;
    }
    if (tilesTotalOverridden) {
        ctx.fieldmapConfig.numTilesTotal = tilesTotalOverride;
    }
    if (metatilesPrimaryOverridden) {
        ctx.fieldmapConfig.numMetatilesInPrimary = metatilesPrimaryOverride;
    }
    if (metatilesTotalOverridden) {
        ctx.fieldmapConfig.numMetatilesTotal = metatilesTotalOverride;
    }
    if (palettesPrimaryOverridden) {
        ctx.fieldmapConfig.numPalettesInPrimary = palettesPrimaryOverride;
    }
    if (palettesTotalOverridden) {
        ctx.fieldmapConfig.numPalettesTotal = palettesTotalOverride;
    }

    if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
        ctx.validateFieldmapParameters(CompilerMode::PRIMARY);
    } else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        ctx.validateFieldmapParameters(CompilerMode::SECONDARY);
    } else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY) {
        ctx.validateFieldmapParameters(DecompilerMode::PRIMARY);
    } else if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
        ctx.validateFieldmapParameters(DecompilerMode::SECONDARY);
    } else {
        Panic("cli_parser::parseSubcommandOptions unknown subcommand");
    }

    /*
     * Die if any errors occurred
     */
    if (ctx.diag->InFlightCountForLevel(DiagLevel::Error) > 0) {
        if (ctx.subcommand == Subcommand::COMPILE_PRIMARY || ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
            die(ctx, "Errors generated during command line parsing. Compilation terminated.");
        }
        die(ctx, "Errors generated during command line parsing. Decompilation terminated.");
    }
}
} // namespace porytiles

#ifndef DOCTEST_CONFIG_DISABLE
TEST_CASE("parseCompile should work as expected with all command lines") {
    // These tests are full of disgusting and evil hacks, avert your gaze
    SUBCASE("Check that the defaults are correct") {
        porytiles::PorytilesContext ctx{};
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;

        optind = 1;

        char bufCmd[64];
        strcpy(bufCmd, "compile-primary");

        char bufPath[64];
        strcpy(bufPath, "/home/foo/pokeemerald");

        char bufHeader[64];
        strcpy(bufHeader, "/home/foo/metatile_behaviors.h");

        char *const argv[] = {bufCmd, bufPath, bufHeader};
        parseSubcommandOptions(ctx, 3, argv);
    }

    SUBCASE("-Wall should enable everything") {
        porytiles::PorytilesContext ctx{};
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;

        optind = 1;

        char bufCmd[64];
        strcpy(bufCmd, "compile-primary");

        char bufWall[64];
        strcpy(bufWall, "-Wall");

        char bufPath[64];
        strcpy(bufPath, "/home/foo/pokeemerald");

        char bufHeader[64];
        strcpy(bufHeader, "/home/foo/metatile_behaviors.h");

        char *const argv[] = {bufCmd, bufWall, bufPath, bufHeader};
        parseSubcommandOptions(ctx, 4, argv);
    }

    SUBCASE("-Wall -Werror should enable everything as an error") {
        porytiles::PorytilesContext ctx{};
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;

        optind = 1;

        char bufCmd[64];
        strcpy(bufCmd, "compile-primary");

        char bufWall[64];
        strcpy(bufWall, "-Wall");

        char bufWerror[64];
        strcpy(bufWerror, "-Werror");

        char bufPath[64];
        strcpy(bufPath, "/home/foo/pokeemerald");

        char bufHeader[64];
        strcpy(bufHeader, "/home/foo/metatile_behaviors.h");

        char *const argv[] = {bufCmd, bufWall, bufWerror, bufPath, bufHeader};
        parseSubcommandOptions(ctx, 5, argv);
    }

    SUBCASE("Should enable a non-default warn, set all to error, then disable the error") {
        porytiles::PorytilesContext ctx{};
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;

        optind = 1;

        char bufCmd[64];
        strcpy(bufCmd, "compile-primary");

        char bufTrueColor[64];
        strcpy(bufTrueColor, "-Wattribute-format-mismatch");

        char bufWerror[64];
        strcpy(bufWerror, "-Werror");

        char bufNoError[64];
        strcpy(bufNoError, "-Wno-error=attribute-format-mismatch");

        char bufPath[64];
        strcpy(bufPath, "/home/foo/pokeemerald");

        char bufHeader[64];
        strcpy(bufHeader, "/home/foo/metatile_behaviors.h");

        char *const argv[] = {bufCmd, bufTrueColor, bufWerror, bufNoError, bufPath, bufHeader};
        parseSubcommandOptions(ctx, 6, argv);
    }

    SUBCASE("Should enable all warnings, then disable one of them") {
        porytiles::PorytilesContext ctx{};
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;

        optind = 1;

        char bufCmd[64];
        strcpy(bufCmd, "compile-primary");

        char bufWall[64];
        strcpy(bufWall, "-Wall");

        char bufNoColorPrecisionLoss[64];
        strcpy(bufNoColorPrecisionLoss, "-Wno-color-precision-loss");

        char bufPath[64];
        strcpy(bufPath, "/home/foo/pokeemerald");

        char bufHeader[64];
        strcpy(bufHeader, "/home/foo/metatile_behaviors.h");

        char *const argv[] = {bufCmd, bufWall, bufNoColorPrecisionLoss, bufPath, bufHeader};
        parseSubcommandOptions(ctx, 5, argv);
    }

    SUBCASE("Global warning disable should work, even if a warning was explicitly enabled") {
        porytiles::PorytilesContext ctx{};
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;

        optind = 1;

        char bufCmd[64];
        strcpy(bufCmd, "compile-primary");

        char bufWnone[64];
        strcpy(bufWnone, "-Wnone");

        char bufPath[64];
        strcpy(bufPath, "/home/foo/pokeemerald");

        char bufHeader[64];
        strcpy(bufHeader, "/home/foo/metatile_behaviors.h");

        char *const argv[] = {bufCmd, bufWnone, bufPath, bufHeader};
        parseSubcommandOptions(ctx, 4, argv);
    }
}
#endif // DOCTEST_CONFIG_DISABLE
