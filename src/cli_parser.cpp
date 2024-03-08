#include "cli_parser.h"

#include <doctest.h>
#include <getopt.h>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>

#define FMT_HEADER_ONLY
#include <fmt/color.h>

#include "build_version.h"
#include "cli_options.h"
#include "errors_warnings.h"
#include "logger.h"
#include "palette_assignment.h"
#include "porytiles_exception.h"
#include "utilities.h"

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

const std::string GLOBAL_HELP =
"porytiles " + std::string{PORYTILES_BUILD_VERSION} + " " + std::string{PORYTILES_BUILD_DATE} + "\n"
"grunt-lucas <grunt.lucas@yahoo.com>\n"
"\n"
"Overworld tileset compiler for use with the pokeruby, pokefirered, and pokeemerald Pokémon\n"
"Generation 3 decompilation projects from pret. Also compatible with pokeemerald-expansion from\n"
"rh-hideout. Builds Porymap-ready tilesets from RGBA (or indexed) tile assets.\n"
"\n"
"Project home page: https://github.com/grunt-lucas/porytiles\n"
"\n"
"\n"
"USAGE\n"
"    porytiles [GLOBAL OPTIONS] SUBCOMMAND [OPTIONS] [ARGS ...]\n"
"    porytiles --help\n"
"    porytiles --version\n"
"\n"
"GLOBAL OPTIONS\n" +
HELP_DESC + "\n" +
VERBOSE_DESC + "\n" +
VERSION_DESC + "\n"
"SUBCOMMANDS\n"
"    " + COMPILE_PRIMARY_COMMAND + "\n"
"        Compile a primary tileset. All files are generated in-place at the output location.\n"
"        Compilation transforms RGBA (or indexed) tile assets into a Porymap-ready tileset.\n"
"\n"
"    " + COMPILE_SECONDARY_COMMAND + "\n"
"        Compile a secondary tileset. All files are generated in-place at the output location.\n"
"        Compilation transforms RGBA (or indexed) tile assets into a Porymap-ready tileset.\n"
"\n"
"    " + DECOMPILE_PRIMARY_COMMAND + "\n"
"        Decompile a primary tileset. All files are generated in-place at the output location.\n"
"        Decompilation transforms a Porymap-ready tileset into RGBA tile assets.\n"
"\n"
"    " + DECOMPILE_SECONDARY_COMMAND + "\n"
"        Decompile a secondary tileset. All files are generated in-place at the output location.\n"
"        Decompilation transforms a Porymap-ready tileset into RGBA tile assets.\n"
"\n"
"Run `porytiles SUBCOMMAND --help' for more information about a subcommand and its OPTIONS and ARGS.\n"
"\n"
"To get more help with Porytiles, check out the guides at:\n"
"    https://github.com/grunt-lucas/porytiles/wiki\n"
"    TODO 1.0.0 : YouTube tutorial playlist link\n"
"\n"
"SEE ALSO\n"
"    https://github.com/pret/pokeruby\n"
"    https://github.com/pret/pokefirered\n"
"    https://github.com/pret/pokeemerald\n"
"    https://github.com/rh-hideout/pokeemerald-expansion\n"
"    https://github.com/huderlem/porymap\n"
"\n";

const std::string COMPILATION_INPUT_DIRECTORY_FORMAT =
"    Compilation Input Directory Format\n"
"        The compilation input directory must conform to the following format. `[]' indicates\n"
"        optional assets.\n"
"            src/\n"
"                bottom.png               # bottom metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                middle.png               # middle metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                top.png                  # top metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                [assign.cache]           # cached configuration for palette assignment algorithm\n"
"                [attributes.csv]         # missing metatile entries will receive default values\n"
"                [anim/]                  # `anim' folder is optional\n"
"                    [anim1/]             # animation names can be arbitrary, but must be unique\n"
"                        key.png          # you must specify a key frame PNG for each anim\n"
"                        00.png           # you must specify at least one animation frame for each anim\n"
"                        [01.png]         # frames must be named numerically, in order\n"
"                        ...              # you may specify an arbitrary number of additional frames\n"
"                    ...                  # you may specify an arbitrary number of additional animations\n"
"                [palette-primers]        # `palette-primers' folder is optional\n"
"                    [foliage.pal]        # e.g. a pal file containing all the colors for your trees and grass\n"
"                    ...                  # you may specify an arbitrary number of additional primer palettes\n";

const std::string DECOMPILATION_INPUT_DIRECTORY_FORMAT =
"    Decompilation Input Directory Format\n"
"        The decompilation input directory must conform to the following format. `[]' indicates\n"
"        optional assets.\n"
"            bin/\n"
"                metatile_attributes.bin  # binary file containing attributes of each metatile\n"
"                metatiles.bin            # binary file containing metatile entries\n"
"                tiles.png                # indexed png of raw tiles\n"
"                palettes                 # directory of palette files\n"
"                    00.pal               # JASC pal file for palette 0\n"
"                    ...                  # there must be each JASC palette file up to NUM_PALS_TOTAL\n"
"                [anim/]                  # `anim' folder is optional\n"
"                    [anim1/]             # animation names can be arbitrary, but must be unique\n"
"                        00.png           # you must specify at least one animation frame for each anim\n"
"                        [01.png]         # frames must be named numerically, in order\n"
"                        ...              # you may specify an arbitrary number of additional frames\n"
"                    ...                  # you may specify an arbitrary number of additional animations\n";

const std::string WARN_OPTIONS_HEADER =
"    Warning Options\n"
"        Use these options to enable or disable additional warnings, as well as set specific\n"
"        warnings as errors. For more information and a full list of available warnings, check:\n"
"        https://github.com/grunt-lucas/porytiles/wiki/Warnings-and-Errors\n";

const std::string COMPILE_PRIMARY_HELP =
"USAGE\n"
"    porytiles " + COMPILE_PRIMARY_COMMAND + " [OPTIONS] INPUT-PATH BEHAVIORS-HEADER\n"
"\n"
"Compile RGBA tile assets into a Porymap-ready primary tileset. `compile-primary' expects an input\n"
"path containing the target assets organized according to the format outlined in the Compilation\n"
"Input Directory Format subsection. You must also supply your project's `metatile_behaviors.h' file.\n"
"By default, `compile-primary' will write output to the current working directory, but you can\n"
"change this behavior by supplying the `-o' option.\n"
"\n"
"ARGS\n"
"    <INPUT-PATH>\n"
"        Path to a directory containing the RGBA tile assets for the target primary set. The\n"
"        directory must conform to the Compilation Input Directory Format outlined below. This\n"
"        tileset is the `target tileset.'\n"
"\n"
"    <BEHAVIORS-HEADER>\n"
"        Path to your project's `metatile_behaviors.h' file. This file is likely located in your\n"
"        project's `include/constants' folder.\n"
"\n" +
COMPILATION_INPUT_DIRECTORY_FORMAT +
"\n"
"OPTIONS\n" +
"    For more detailed information about the options below, check out the options pages here:\n" +
"    https://github.com/grunt-lucas/porytiles/wiki#advanced-usage\n" +
"\n" +
"    Driver Options\n" +
OUTPUT_DESC + "\n" +
TILES_OUTPUT_PAL_DESC + "\n" +
DISABLE_METATILE_GENERATION_DESC + "\n" +
DISABLE_ATTRIBUTE_GENERATION_DESC + "\n" +
"    Tileset Compilation Options\n" +
TARGET_BASE_GAME_DESC + "\n" +
DUAL_LAYER_DESC + "\n" +
TRANSPARENCY_COLOR_DESC + "\n" +
DEFAULT_BEHAVIOR_DESC + "\n" +
DEFAULT_ENCOUNTER_TYPE_DESC + "\n" +
DEFAULT_TERRAIN_TYPE_DESC + "\n" +
"    Palette Assignment Config Options\n" +
ASSIGN_ALGO_DESC + "\n" +
EXPLORE_CUTOFF_DESC + "\n" +
BEST_BRANCHES_DESC + "\n" +
DISABLE_ASSIGN_CACHING_DESC + "\n" +
FORCE_ASSIGN_PARAM_MATRIX_DESC + "\n" +
"    Fieldmap Override Options\n" +
TILES_PRIMARY_OVERRIDE_DESC + "\n" +
TILES_TOTAL_OVERRIDE_DESC + "\n" +
METATILES_PRIMARY_OVERRIDE_DESC + "\n" +
METATILES_TOTAL_OVERRIDE_DESC + "\n" +
PALS_PRIMARY_OVERRIDE_DESC + "\n" +
PALS_TOTAL_OVERRIDE_DESC + "\n" +
WARN_OPTIONS_HEADER +
"\n" +
WALL_DESC + "\n" +
WNONE_DESC + "\n" +
W_GENERAL_DESC + "\n" +
WERROR_DESC + "\n";

const std::string COMPILE_SECONDARY_HELP =
"USAGE\n"
"    porytiles " + COMPILE_SECONDARY_COMMAND + " [OPTIONS] INPUT-PATH PRIMARY-INPUT-PATH BEHAVIORS-HEADER\n"
"\n"
"Compile RGBA tile assets into a Porymap-ready secondary tileset. `compile-secondary' expects an\n"
"input path containing the target assets organized according to the format outlined in the\n"
"Compilation Input Directory Format subsection. You must also supply the RGBA tile assets for a\n"
"paired primary tileset, so Porytiles can take advantage of the Generation 3 engine's tile re-use\n"
"system. Like `compile-primary', you must also supply your project's `metatile_behaviors.h' file. By\n"
"default, `compile-secondary' will write output to the current working directory, but you can change\n"
"this behavior by supplying the `-o' option.\n"
"\n"
"ARGS\n"
"    <INPUT-PATH>\n"
"        Path to a directory containing the RGBA tile assets for the target secondary set. The\n"
"        directory must conform to the Compilation Input Directory Format outlined below. This\n"
"        tileset is the `target tileset.'\n"
"\n"
"    <PRIMARY-INPUT-PATH>\n"
"        Path to a directory containing the RGBA tile assets for the paired primary set of the\n"
"        target secondary tileset. The directory must conform to the Compilation Input Directory\n"
"        Format outlined below.\n"
"\n"
"    <BEHAVIORS-HEADER>\n"
"        Path to your project's `metatile_behaviors.h' file. This file is likely located in your\n"
"        project's `include/constants' folder.\n"
"\n" +
COMPILATION_INPUT_DIRECTORY_FORMAT +
"\n"
"OPTIONS\n" +
"    For more detailed information about the options below, check out the options pages here:\n" +
"    https://github.com/grunt-lucas/porytiles/wiki#advanced-usage\n" +
"\n" +
"    Driver Options\n" +
OUTPUT_DESC + "\n" +
TILES_OUTPUT_PAL_DESC + "\n" +
DISABLE_METATILE_GENERATION_DESC + "\n" +
DISABLE_ATTRIBUTE_GENERATION_DESC + "\n" +
"    Tileset Compilation Options\n" +
TARGET_BASE_GAME_DESC + "\n" +
DUAL_LAYER_DESC + "\n" +
TRANSPARENCY_COLOR_DESC + "\n" +
DEFAULT_BEHAVIOR_DESC + "\n" +
DEFAULT_ENCOUNTER_TYPE_DESC + "\n" +
DEFAULT_TERRAIN_TYPE_DESC + "\n" +
"    Palette Assignment Config Options\n" +
ASSIGN_ALGO_DESC + "\n" +
EXPLORE_CUTOFF_DESC + "\n" +
BEST_BRANCHES_DESC + "\n" +
DISABLE_ASSIGN_CACHING_DESC + "\n" +
FORCE_ASSIGN_PARAM_MATRIX_DESC + "\n" +
"    Primary Palette Assignment Config Options\n" +
PRIMARY_ASSIGN_ALGO_DESC + "\n" +
PRIMARY_EXPLORE_CUTOFF_DESC + "\n" +
PRIMARY_BEST_BRANCHES_DESC + "\n" +
"    Fieldmap Override Options\n" +
TILES_PRIMARY_OVERRIDE_DESC + "\n" +
TILES_TOTAL_OVERRIDE_DESC + "\n" +
METATILES_PRIMARY_OVERRIDE_DESC + "\n" +
METATILES_TOTAL_OVERRIDE_DESC + "\n" +
PALS_PRIMARY_OVERRIDE_DESC + "\n" +
PALS_TOTAL_OVERRIDE_DESC + "\n" +
WARN_OPTIONS_HEADER +
"\n" +
WALL_DESC + "\n" +
WNONE_DESC + "\n" +
W_GENERAL_DESC + "\n" +
WERROR_DESC + "\n";

const std::string DECOMPILE_PRIMARY_HELP =
"USAGE\n"
"    porytiles " + DECOMPILE_PRIMARY_COMMAND + " [OPTIONS] INPUT-PATH BEHAVIORS-HEADER\n"
"\n"
"Decompile a Porymap-ready primary tileset back into Porytiles-compatible RGBA tile assets.\n"
"`decompile-primary' expects an input path containing target compiled tile assets organized\n"
"according to the format outlined in the Decompilation Input Directory Format subsection. Like the\n"
"compilation commands, `decompile-primary' requires your project's `metatile_behaviors.h' file. And\n"
"you can control its output location via the `-o' option.\n"
"\n"
"ARGS\n"
"    <INPUT-PATH>\n"
"        Path to a directory containing the compiled primary tileset. The directory must conform\n"
"        to the Decompilation Input Directory Format outlined below. This tileset is the `target\n"
"        tileset.'\n"
"\n"
"    <BEHAVIORS-HEADER>\n"
"        Path to your project's `metatile_behaviors.h' file. This file is likely located in your\n"
"        project's `include/constants' folder.\n"
"\n" +
DECOMPILATION_INPUT_DIRECTORY_FORMAT +
"\n"
"OPTIONS\n" +
"    For more detailed information about the options below, check out the options pages here:\n" +
"    https://github.com/grunt-lucas/porytiles/wiki#advanced-usage\n" +
"\n" +
"    Driver Options\n" +
OUTPUT_DESC + "\n" +
"    Tileset Decompilation Options\n" +
TARGET_BASE_GAME_DESC + "\n" +
NORMALIZE_TRANSPARENCY_DESC + "\n" +
"    Fieldmap Override Options\n" +
TILES_PRIMARY_OVERRIDE_DESC + "\n" +
TILES_TOTAL_OVERRIDE_DESC + "\n" +
PALS_PRIMARY_OVERRIDE_DESC + "\n" +
PALS_TOTAL_OVERRIDE_DESC + "\n" +
WARN_OPTIONS_HEADER +
"\n" +
WALL_DESC + "\n" +
WNONE_DESC + "\n" +
W_GENERAL_DESC + "\n" +
WERROR_DESC + "\n";

const std::string DECOMPILE_SECONDARY_HELP =
"USAGE\n"
"    porytiles " + DECOMPILE_SECONDARY_COMMAND + " [OPTIONS] INPUT-PATH PRIMARY-INPUT-PATH BEHAVIORS-HEADER\n"
"\n"
"Decompile a Porymap-ready secondary tileset back into Porytiles-compatible RGBA tile assets.\n"
"`decompile-secondary' expects an input path containing target compiled tile assets organized\n"
"according to the format outlined in the Decompilation Input Directory Format subsection. You must\n"
"also supply the compiled tile assets of the target tileset's paired primary. `decompile-primary'\n"
"requires your project's `metatile_behaviors.h' file. And you can control its output location via\n"
"the `-o' option.\n"
"\n"
"ARGS\n"
"    <INPUT-PATH>\n"
"        Path to a directory containing the compiled secondary tileset. The directory must conform\n"
"        to the Decompilation Input Directory Format outlined below. This tileset is the `target\n"
"        tileset.'\n"
"\n"
"    <PRIMARY-INPUT-PATH>\n"
"        Path to a directory containing the compiled paired primary tileset for the target secondary\n"
"        tileset. The directory must conform to the Decompilation Input Directory Format outlined\n"
"        below.\n"
"\n"
"    <BEHAVIORS-HEADER>\n"
"        Path to your project's `metatile_behaviors.h' file. This file is likely located in your\n"
"        project's `include/constants' folder.\n"
"\n" +
DECOMPILATION_INPUT_DIRECTORY_FORMAT +
"\n"
"OPTIONS\n" +
"    For more detailed information about the options below, check out the options pages here:\n" +
"    https://github.com/grunt-lucas/porytiles/wiki#advanced-usage\n" +
"\n" +
"    Driver Options\n" +
OUTPUT_DESC + "\n" +
"    Tileset Decompilation Options\n" +
TARGET_BASE_GAME_DESC + "\n" +
NORMALIZE_TRANSPARENCY_DESC + "\n" +
"    Fieldmap Override Options\n" +
TILES_PRIMARY_OVERRIDE_DESC + "\n" +
TILES_TOTAL_OVERRIDE_DESC + "\n" +
PALS_PRIMARY_OVERRIDE_DESC + "\n" +
PALS_TOTAL_OVERRIDE_DESC + "\n" +
WARN_OPTIONS_HEADER +
"\n" +
WALL_DESC + "\n" +
WNONE_DESC + "\n" +
W_GENERAL_DESC + "\n" +
WERROR_DESC + "\n";
// @formatter:on
// clang-format on

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
    {ASSIGN_ALGO, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {EXPLORE_CUTOFF, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {BEST_BRANCHES, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {DISABLE_ASSIGN_CACHING, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {FORCE_ASSIGN_PARAM_MATRIX, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
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
    {WKEY_FRAME_DID_NOT_APPEAR, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_KEY_FRAME_DID_NOT_APPEAR, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WUSED_TRUE_COLOR_MODE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_USED_TRUE_COLOR_MODE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WATTRIBUTE_FORMAT_MISMATCH, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_ATTRIBUTE_FORMAT_MISMATCH, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WMISSING_ASSIGN_CONFIG, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_MISSING_ASSIGN_CONFIG, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WUNUSED_ATTRIBUTE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_UNUSED_ATTRIBUTE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WTRANSPARENCY_COLLAPSE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_TRANSPARENCY_COLLAPSE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WASSIGN_CONFIG_OVERRIDE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_ASSIGN_CONFIG_OVERRIDE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WINVALID_ASSIGN_CONFIG_CACHE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_INVALID_ASSIGN_CONFIG_CACHE, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WMISSING_ASSIGN_CONFIG, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    {WNO_MISSING_ASSIGN_CONFIG, {Subcommand::COMPILE_PRIMARY, Subcommand::COMPILE_SECONDARY}},
    // Decompilation warnings
    {WINVALID_TILE_INDEX, {Subcommand::DECOMPILE_PRIMARY, Subcommand::DECOMPILE_SECONDARY}},
    {WNO_INVALID_TILE_INDEX, {Subcommand::DECOMPILE_PRIMARY, Subcommand::DECOMPILE_SECONDARY}},
    // TODO 1.0.0 : this does not correctly handle the -Werror=foo case where foo is an incompatible warning
};

void parseOptions(PorytilesContext &ctx, int argc, char *const *argv)
{
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
    internalerror("cli_parser::parseOptions unknown subcommand setting");
  }
}

template <typename T>
static T parseIntegralOption(const ErrorsAndWarnings &err, const std::string &optionName, const char *optarg)
{
  try {
    T arg = parseInteger<T>(optarg);
    return arg;
  }
  catch (const std::exception &e) {
    fatalerror(err, fmt::format("invalid argument `{}' for option `{}': {}", fmt::styled(optarg, fmt::emphasis::bold),
                                fmt::styled(optionName, fmt::emphasis::bold), e.what()));
  }
  // unreachable, here for compiler
  throw std::runtime_error("cli_parser::parseIntegralOption reached unreachable code path");
}

static RGBA32 parseRgbColor(const ErrorsAndWarnings &err, std::string optionName, const std::string &colorString)
{
  std::vector<std::string> colorComponents = split(colorString, ",");
  if (colorComponents.size() != 3) {
    fatalerror(err, fmt::format("invalid argument `{}' for option `{}': RGB color must have three components",
                                fmt::styled(colorString, fmt::emphasis::bold),
                                fmt::styled(optionName, fmt::emphasis::bold)));
  }
  int red = parseIntegralOption<int>(err, optionName, colorComponents[0].c_str());
  int green = parseIntegralOption<int>(err, optionName, colorComponents[1].c_str());
  int blue = parseIntegralOption<int>(err, optionName, colorComponents[2].c_str());

  if (red < 0 || red > 255) {
    fatalerror(err, fmt::format("invalid red component `{}' for option `{}': range must be 0 <= red <= 255",
                                fmt::styled(red, fmt::emphasis::bold), fmt::styled(optionName, fmt::emphasis::bold)));
  }
  if (green < 0 || green > 255) {
    fatalerror(err, fmt::format("invalid green component `{}' for option `{}': range must be 0 <= green <= 255",
                                fmt::styled(green, fmt::emphasis::bold), fmt::styled(optionName, fmt::emphasis::bold)));
  }
  if (blue < 0 || blue > 255) {
    fatalerror(err, fmt::format("invalid blue component `{}' for option `{}': range must be 0 <= blue <= 255",
                                fmt::styled(blue, fmt::emphasis::bold), fmt::styled(optionName, fmt::emphasis::bold)));
  }

  return RGBA32{static_cast<std::uint8_t>(red), static_cast<std::uint8_t>(green), static_cast<std::uint8_t>(blue),
                ALPHA_OPAQUE};
}

static TilesOutputPalette parseTilesPngPaletteMode(const ErrorsAndWarnings &err, const std::string &optionName,
                                                   const char *optarg)
{
  std::string optargString{optarg};
  if (optargString == "true-color") {
    return TilesOutputPalette::TRUE_COLOR;
  }
  else if (optargString == "greyscale") {
    return TilesOutputPalette::GREYSCALE;
  }
  else {
    fatalerror(err, fmt::format("invalid argument `{}' for option `{}'", fmt::styled(optargString, fmt::emphasis::bold),
                                fmt::styled(optionName, fmt::emphasis::bold)));
  }
  // unreachable, here for compiler
  throw std::runtime_error("cli_parser::parseTilesPngPaletteMode reached unreachable code path");
}

static TargetBaseGame parseTargetBaseGame(const ErrorsAndWarnings &err, const std::string &optionName,
                                          const char *optarg)
{
  std::string optargString{optarg};
  if (optargString == "pokeemerald") {
    return TargetBaseGame::EMERALD;
  }
  else if (optargString == "pokefirered") {
    return TargetBaseGame::FIRERED;
  }
  else if (optargString == "pokeruby") {
    return TargetBaseGame::RUBY;
  }
  else {
    fatalerror(err, fmt::format("invalid argument `{}' for option `{}'", fmt::styled(optargString, fmt::emphasis::bold),
                                fmt::styled(optionName, fmt::emphasis::bold)));
  }
  // unreachable, here for compiler
  throw std::runtime_error("cli_parser::parseTargetBaseGame reached unreachable code path");
}

static AssignAlgorithm parseAssignAlgorithm(const ErrorsAndWarnings &err, const std::string &optionName,
                                            const char *optarg)
{
  std::string optargString{optarg};
  if (optargString == assignAlgorithmString(AssignAlgorithm::DFS)) {
    return AssignAlgorithm::DFS;
  }
  else if (optargString == assignAlgorithmString(AssignAlgorithm::BFS)) {
    return AssignAlgorithm::BFS;
  }
  else {
    fatalerror(err, fmt::format("invalid argument `{}' for option `{}'", fmt::styled(optargString, fmt::emphasis::bold),
                                fmt::styled(optionName, fmt::emphasis::bold)));
  }
  // unreachable, here for compiler
  throw std::runtime_error("cli_parser::parseAssignAlgorithm reached unreachable code path");
}

const std::vector<std::string> GLOBAL_SHORTS = {};
static void parseGlobalOptions(PorytilesContext &ctx, int argc, char *const *argv)
{
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

static void parseSubcommand(PorytilesContext &ctx, int argc, char *const *argv)
{
  if ((argc - optind) == 0) {
    fatalerror(ctx.err, "missing required subcommand, try `porytiles --help' for usage information");
  }

  std::string subcommand = argv[optind++];
  if (subcommand == DECOMPILE_PRIMARY_COMMAND) {
    ctx.subcommand = Subcommand::DECOMPILE_PRIMARY;
  }
  else if (subcommand == DECOMPILE_SECONDARY_COMMAND) {
    ctx.subcommand = Subcommand::DECOMPILE_SECONDARY;
  }
  else if (subcommand == COMPILE_PRIMARY_COMMAND) {
    ctx.subcommand = Subcommand::COMPILE_PRIMARY;
  }
  else if (subcommand == COMPILE_SECONDARY_COMMAND) {
    ctx.subcommand = Subcommand::COMPILE_SECONDARY;
  }
  else {
    fatalerror(ctx.err, "unrecognized subcommand `" + subcommand + "', try `porytiles --help' for usage information");
  }
}

static void validateSubcommandContext(PorytilesContext &ctx, std::string option)
{
  if (!supportedSubcommands.contains(option)) {
    internalerror(fmt::format("`supportedSubcommands' did not contain mapping for option `{}'", option));
  }
  if (!supportedSubcommands.at(option).contains(ctx.subcommand)) {
    fatalerror_unrecognizedOption(ctx.err, option, ctx.subcommand);
  }
}

const std::vector<std::string> COMPILE_SHORTS = {};
/*
 * FIXME : the warning parsing system here is a dumpster fire
 */
static void parseSubcommandOptions(PorytilesContext &ctx, int argc, char *const *argv)
{
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
      {DISABLE_ASSIGN_CACHING.c_str(), no_argument, nullptr, DISABLE_ASSIGN_CACHING_VAL},
      {FORCE_ASSIGN_PARAM_MATRIX.c_str(), no_argument, nullptr, FORCE_ASSIGN_PARAM_MATRIX_VAL},
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

      {WKEY_FRAME_DID_NOT_APPEAR.c_str(), no_argument, nullptr, WKEY_FRAME_DID_NOT_APPEAR_VAL},
      {WNO_KEY_FRAME_DID_NOT_APPEAR.c_str(), no_argument, nullptr, WNO_KEY_FRAME_DID_NOT_APPEAR_VAL},

      {WUSED_TRUE_COLOR_MODE.c_str(), no_argument, nullptr, WUSED_TRUE_COLOR_MODE_VAL},
      {WNO_USED_TRUE_COLOR_MODE.c_str(), no_argument, nullptr, WNO_USED_TRUE_COLOR_MODE_VAL},

      {WATTRIBUTE_FORMAT_MISMATCH.c_str(), no_argument, nullptr, WATTRIBUTE_FORMAT_MISMATCH_VAL},
      {WNO_ATTRIBUTE_FORMAT_MISMATCH.c_str(), no_argument, nullptr, WNO_ATTRIBUTE_FORMAT_MISMATCH_VAL},

      {WMISSING_ATTRIBUTES_CSV.c_str(), no_argument, nullptr, WMISSING_ATTRIBUTES_CSV_VAL},
      {WNO_MISSING_ATTRIBUTES_CSV.c_str(), no_argument, nullptr, WNO_MISSING_ATTRIBUTES_CSV_VAL},

      {WUNUSED_ATTRIBUTE.c_str(), no_argument, nullptr, WUNUSED_ATTRIBUTE_VAL},
      {WNO_UNUSED_ATTRIBUTE.c_str(), no_argument, nullptr, WNO_UNUSED_ATTRIBUTE_VAL},

      {WTRANSPARENCY_COLLAPSE.c_str(), no_argument, nullptr, WTRANSPARENCY_COLLAPSE_VAL},
      {WNO_TRANSPARENCY_COLLAPSE.c_str(), no_argument, nullptr, WNO_TRANSPARENCY_COLLAPSE_VAL},

      {WASSIGN_CONFIG_OVERRIDE.c_str(), no_argument, nullptr, WASSIGN_CONFIG_OVERRIDE_VAL},
      {WNO_ASSIGN_CONFIG_OVERRIDE.c_str(), no_argument, nullptr, WNO_ASSIGN_CONFIG_OVERRIDE_VAL},

      {WINVALID_ASSIGN_CONFIG_CACHE.c_str(), no_argument, nullptr, WINVALID_ASSIGN_CONFIG_CACHE_VAL},
      {WNO_INVALID_ASSIGN_CONFIG_CACHE.c_str(), no_argument, nullptr, WNO_INVALID_ASSIGN_CONFIG_CACHE_VAL},

      {WMISSING_ASSIGN_CONFIG.c_str(), no_argument, nullptr, WMISSING_ASSIGN_CONFIG_VAL},
      {WNO_MISSING_ASSIGN_CONFIG.c_str(), no_argument, nullptr, WNO_MISSING_ASSIGN_CONFIG_VAL},

      // Decompilation warnings
      {WINVALID_TILE_INDEX.c_str(), no_argument, nullptr, WINVALID_TILE_INDEX_VAL},
      {WNO_INVALID_TILE_INDEX.c_str(), no_argument, nullptr, WNO_INVALID_TILE_INDEX_VAL},

      // Help
      {HELP.c_str(), no_argument, nullptr, HELP_VAL},
      {HELP_SHORT.c_str(), no_argument, nullptr, HELP_VAL},

      {nullptr, no_argument, nullptr, 0}};

  /*
   * Warning specific variables. We must wait until after all options are processed before we actually enable warnings,
   * since enabling/disabling specific warnings must take precedence over the general -Wall and -Werror flags no matter
   * where in the command line the user specified. Some of these warnings are enabled by default.
   */
  bool enableAllWarnings = false;
  bool disableAllWarnings = false;
  bool setAllEnabledWarningsToErrors = false;

  // Compilation warnings
  std::optional<bool> warnColorPrecisionLossOverride{};
  std::optional<bool> errColorPrecisionLossOverride{};

  std::optional<bool> warnKeyFrameTileDidNotAppearInAssignmentOverride{};
  std::optional<bool> errKeyFrameTileDidNotAppearInAssignmentOverride{};

  // TODO : Porymap now supports but needs latest version, a future release should default this warning to OFF
  std::optional<bool> warnUsedTrueColorModeOverride{true};
  std::optional<bool> errUsedTrueColorModeOverride{};

  std::optional<bool> warnAttributeFormatMismatchOverride{};
  std::optional<bool> errAttributeFormatMismatchOverride{};

  std::optional<bool> warnMissingAttributesCsvOverride{};
  std::optional<bool> errMissingAttributesCsvOverride{};

  std::optional<bool> warnUnusedAttributeOverride{};
  std::optional<bool> errUnusedAttributeOverride{};

  std::optional<bool> warnTransparencyCollapseOverride{};
  std::optional<bool> errTransparencyCollapseOverride{};

  std::optional<bool> warnAssignCacheOverride{};
  std::optional<bool> errAssignCacheOverride{};

  std::optional<bool> warnInvalidAssignCache{};
  std::optional<bool> errInvalidAssignCache{};

  std::optional<bool> warnMissingAssignCache{};
  std::optional<bool> errMissingAssignCache{};

  // Decompilation warnings
  std::optional<bool> warnInvalidTileIndex{};
  std::optional<bool> errInvalidTileIndex{};

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
      ctx.output.paletteMode = parseTilesPngPaletteMode(ctx.err, TILES_OUTPUT_PAL, optarg);
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
      ctx.targetBaseGame = parseTargetBaseGame(ctx.err, TARGET_BASE_GAME, optarg);
      break;
    case DUAL_LAYER_VAL:
      validateSubcommandContext(ctx, DUAL_LAYER);
      ctx.compilerConfig.tripleLayer = false;
      break;
    case TRANSPARENCY_COLOR_VAL:
      validateSubcommandContext(ctx, TRANSPARENCY_COLOR);
      ctx.compilerConfig.transparencyColor = parseRgbColor(ctx.err, TRANSPARENCY_COLOR, optarg);
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
      if (optarg != NULL) {
        ctx.decompilerConfig.normalizeTransparencyColor = parseRgbColor(ctx.err, NORMALIZE_TRANSPARENCY, optarg);
      }
      break;

    // Color assignment config options
    case EXPLORE_CUTOFF_VAL:
      validateSubcommandContext(ctx, EXPLORE_CUTOFF);
      ctx.compilerConfig.providedAssignCacheOverride = true;
      exploreCutoff = parseIntegralOption<std::size_t>(ctx.err, EXPLORE_CUTOFF, optarg);
      if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
        ctx.compilerConfig.primaryExploredNodeCutoff = exploreCutoff;
        if (ctx.compilerConfig.primaryExploredNodeCutoff > EXPLORATION_MAX_CUTOFF) {
          fatalerror(ctx.err, fmt::format("option `{}' argument cannot be > 100",
                                          fmt::styled(EXPLORE_CUTOFF, fmt::emphasis::bold)));
        }
      }
      else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        ctx.compilerConfig.secondaryExploredNodeCutoff = exploreCutoff;
        if (ctx.compilerConfig.secondaryExploredNodeCutoff > EXPLORATION_MAX_CUTOFF) {
          fatalerror(ctx.err, fmt::format("option `{}' argument cannot be > 100",
                                          fmt::styled(EXPLORE_CUTOFF, fmt::emphasis::bold)));
        }
      }
      break;
    case ASSIGN_ALGO_VAL:
      validateSubcommandContext(ctx, ASSIGN_ALGO);
      ctx.compilerConfig.providedAssignCacheOverride = true;
      if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
        ctx.compilerConfig.primaryAssignAlgorithm = parseAssignAlgorithm(ctx.err, ASSIGN_ALGO, optarg);
      }
      else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        ctx.compilerConfig.secondaryAssignAlgorithm = parseAssignAlgorithm(ctx.err, ASSIGN_ALGO, optarg);
      }
      break;
    case BEST_BRANCHES_VAL:
      validateSubcommandContext(ctx, BEST_BRANCHES);
      ctx.compilerConfig.providedAssignCacheOverride = true;
      if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
        if (std::string{optarg} == SMART_PRUNE) {
          ctx.compilerConfig.primarySmartPrune = true;
        }
        else {
          ctx.compilerConfig.primaryBestBranches = parseIntegralOption<std::size_t>(ctx.err, BEST_BRANCHES, optarg);
          if (ctx.compilerConfig.primaryBestBranches == 0) {
            fatalerror(ctx.err, fmt::format("option `{}' argument cannot be 0",
                                            fmt::styled(BEST_BRANCHES, fmt::emphasis::bold)));
          }
        }
      }
      else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        if (std::string{optarg} == SMART_PRUNE) {
          ctx.compilerConfig.secondarySmartPrune = true;
        }
        else {
          ctx.compilerConfig.secondaryBestBranches = parseIntegralOption<std::size_t>(ctx.err, BEST_BRANCHES, optarg);
          if (ctx.compilerConfig.secondaryBestBranches == 0) {
            fatalerror(ctx.err, fmt::format("option `{}' argument cannot be 0",
                                            fmt::styled(BEST_BRANCHES, fmt::emphasis::bold)));
          }
        }
      }
      break;
    case DISABLE_ASSIGN_CACHING_VAL:
      validateSubcommandContext(ctx, DISABLE_ASSIGN_CACHING);
      ctx.compilerConfig.cacheAssign = false;
      break;
    case FORCE_ASSIGN_PARAM_MATRIX_VAL:
      validateSubcommandContext(ctx, FORCE_ASSIGN_PARAM_MATRIX);
      ctx.compilerConfig.forceParamSearchMatrix = true;
      break;
    case PRIMARY_EXPLORE_CUTOFF_VAL:
      validateSubcommandContext(ctx, PRIMARY_EXPLORE_CUTOFF);
      ctx.compilerConfig.providedPrimaryAssignCacheOverride = true;
      exploreCutoff = parseIntegralOption<std::size_t>(ctx.err, PRIMARY_EXPLORE_CUTOFF, optarg);
      if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        ctx.compilerConfig.primaryExploredNodeCutoff = exploreCutoff;
        if (ctx.compilerConfig.primaryExploredNodeCutoff > EXPLORATION_MAX_CUTOFF) {
          fatalerror(ctx.err, fmt::format("option `{}' argument cannot be > 100",
                                          fmt::styled(PRIMARY_EXPLORE_CUTOFF, fmt::emphasis::bold)));
        }
      }
      break;
    case PRIMARY_ASSIGN_ALGO_VAL:
      validateSubcommandContext(ctx, PRIMARY_ASSIGN_ALGO);
      ctx.compilerConfig.providedPrimaryAssignCacheOverride = true;
      if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        ctx.compilerConfig.primaryAssignAlgorithm = parseAssignAlgorithm(ctx.err, PRIMARY_ASSIGN_ALGO, optarg);
      }
      break;
    case PRIMARY_BEST_BRANCHES_VAL:
      validateSubcommandContext(ctx, PRIMARY_BEST_BRANCHES);
      ctx.compilerConfig.providedPrimaryAssignCacheOverride = true;
      if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        if (std::string{optarg} == "smart") {
          ctx.compilerConfig.primarySmartPrune = true;
        }
        else {
          ctx.compilerConfig.primaryBestBranches =
              parseIntegralOption<std::size_t>(ctx.err, PRIMARY_BEST_BRANCHES, optarg);
          if (ctx.compilerConfig.primaryBestBranches == 0) {
            fatalerror(ctx.err, fmt::format("option `{}' argument cannot be 0",
                                            fmt::styled(PRIMARY_BEST_BRANCHES, fmt::emphasis::bold)));
          }
        }
      }
      break;

    // Fieldmap override options
    case TILES_PRIMARY_OVERRIDE_VAL:
      validateSubcommandContext(ctx, TILES_PRIMARY_OVERRIDE);
      tilesPrimaryOverridden = true;
      tilesPrimaryOverride = parseIntegralOption<std::size_t>(ctx.err, TILES_PRIMARY_OVERRIDE, optarg);
      break;
    case TILES_TOTAL_OVERRIDE_VAL:
      validateSubcommandContext(ctx, TILES_TOTAL_OVERRIDE);
      tilesTotalOverridden = true;
      tilesTotalOverride = parseIntegralOption<std::size_t>(ctx.err, TILES_TOTAL_OVERRIDE, optarg);
      break;
    case METATILES_PRIMARY_OVERRIDE_VAL:
      validateSubcommandContext(ctx, METATILES_PRIMARY_OVERRIDE);
      metatilesPrimaryOverridden = true;
      metatilesPrimaryOverride = parseIntegralOption<std::size_t>(ctx.err, METATILES_PRIMARY_OVERRIDE, optarg);
      break;
    case METATILES_TOTAL_OVERRIDE_VAL:
      validateSubcommandContext(ctx, METATILES_TOTAL_OVERRIDE);
      metatilesTotalOverridden = true;
      metatilesTotalOverride = parseIntegralOption<std::size_t>(ctx.err, METATILES_TOTAL_OVERRIDE, optarg);
      break;
    case PALS_PRIMARY_OVERRIDE_VAL:
      validateSubcommandContext(ctx, PALS_PRIMARY_OVERRIDE);
      palettesPrimaryOverridden = true;
      palettesPrimaryOverride = parseIntegralOption<std::size_t>(ctx.err, PALS_PRIMARY_OVERRIDE, optarg);
      break;
    case PALS_TOTAL_OVERRIDE_VAL:
      validateSubcommandContext(ctx, PALS_TOTAL_OVERRIDE);
      palettesTotalOverridden = true;
      palettesTotalOverride = parseIntegralOption<std::size_t>(ctx.err, PALS_TOTAL_OVERRIDE, optarg);
      break;

    // Warning and error options
    case WALL_VAL:
      validateSubcommandContext(ctx, WALL);
      enableAllWarnings = true;
      break;
    case WNONE_VAL:
      validateSubcommandContext(ctx, WNONE);
      disableAllWarnings = true;
      break;
    case WERROR_VAL:
      validateSubcommandContext(ctx, WERROR);
      if (optarg == NULL) {
        setAllEnabledWarningsToErrors = true;
      }
      else {
        // Compilation warnings
        if (strcmp(optarg, WARN_COLOR_PRECISION_LOSS) == 0) {
          errColorPrecisionLossOverride = true;
        }
        else if (strcmp(optarg, WARN_KEY_FRAME_DID_NOT_APPEAR) == 0) {
          errKeyFrameTileDidNotAppearInAssignmentOverride = true;
        }
        else if (strcmp(optarg, WARN_USED_TRUE_COLOR_MODE) == 0) {
          errUsedTrueColorModeOverride = true;
        }
        else if (strcmp(optarg, WARN_ATTRIBUTE_FORMAT_MISMATCH) == 0) {
          errAttributeFormatMismatchOverride = true;
        }
        else if (strcmp(optarg, WARN_MISSING_ATTRIBUTES_CSV) == 0) {
          errMissingAttributesCsvOverride = true;
        }
        else if (strcmp(optarg, WARN_UNUSED_ATTRIBUTE) == 0) {
          errUnusedAttributeOverride = true;
        }
        else if (strcmp(optarg, WARN_TRANSPARENCY_COLLAPSE) == 0) {
          errTransparencyCollapseOverride = true;
        }
        else if (strcmp(optarg, WARN_ASSIGN_CACHE_OVERRIDE) == 0) {
          errAssignCacheOverride = true;
        }
        else if (strcmp(optarg, WARN_INVALID_ASSIGN_CACHE) == 0) {
          errInvalidAssignCache = true;
        }
        else if (strcmp(optarg, WARN_MISSING_ASSIGN_CACHE) == 0) {
          errMissingAssignCache = true;
        }
        // Decompilation warnings
        else if (strcmp(optarg, WARN_INVALID_TILE_INDEX) == 0) {
          errInvalidTileIndex = true;
        }
        else {
          fatalerror(ctx.err, fmt::format("invalid argument `{}' for option `{}'",
                                          fmt::styled(std::string{optarg}, fmt::emphasis::bold),
                                          fmt::styled(WERROR, fmt::emphasis::bold)));
        }
      }
      break;
    case WNO_ERROR_VAL:
      validateSubcommandContext(ctx, WNO_ERROR);
      // Compilation warnings
      if (strcmp(optarg, WARN_COLOR_PRECISION_LOSS) == 0) {
        errColorPrecisionLossOverride = false;
      }
      else if (strcmp(optarg, WARN_KEY_FRAME_DID_NOT_APPEAR) == 0) {
        errKeyFrameTileDidNotAppearInAssignmentOverride = false;
      }
      else if (strcmp(optarg, WARN_USED_TRUE_COLOR_MODE) == 0) {
        errUsedTrueColorModeOverride = false;
      }
      else if (strcmp(optarg, WARN_ATTRIBUTE_FORMAT_MISMATCH) == 0) {
        errAttributeFormatMismatchOverride = false;
      }
      else if (strcmp(optarg, WARN_MISSING_ATTRIBUTES_CSV) == 0) {
        errMissingAttributesCsvOverride = false;
      }
      else if (strcmp(optarg, WARN_UNUSED_ATTRIBUTE) == 0) {
        errUnusedAttributeOverride = false;
      }
      else if (strcmp(optarg, WARN_TRANSPARENCY_COLLAPSE) == 0) {
        errTransparencyCollapseOverride = false;
      }
      else if (strcmp(optarg, WARN_ASSIGN_CACHE_OVERRIDE) == 0) {
        errAssignCacheOverride = false;
      }
      else if (strcmp(optarg, WARN_INVALID_ASSIGN_CACHE) == 0) {
        errInvalidAssignCache = false;
      }
      else if (strcmp(optarg, WARN_MISSING_ASSIGN_CACHE) == 0) {
        errMissingAssignCache = false;
      }
      // Decompilation warnings
      else if (strcmp(optarg, WARN_INVALID_TILE_INDEX) == 0) {
        errInvalidTileIndex = false;
      }
      else {
        fatalerror(ctx.err, fmt::format("invalid argument `{}' for option `{}'",
                                        fmt::styled(std::string{optarg}, fmt::emphasis::bold),
                                        fmt::styled(WERROR, fmt::emphasis::bold)));
      }
      break;

    // Compilation warnings
    case WCOLOR_PRECISION_LOSS_VAL:
      validateSubcommandContext(ctx, WCOLOR_PRECISION_LOSS);
      warnColorPrecisionLossOverride = true;
      break;
    case WNO_COLOR_PRECISION_LOSS_VAL:
      validateSubcommandContext(ctx, WNO_COLOR_PRECISION_LOSS);
      warnColorPrecisionLossOverride = false;
      break;
    case WKEY_FRAME_DID_NOT_APPEAR_VAL:
      validateSubcommandContext(ctx, WKEY_FRAME_DID_NOT_APPEAR);
      warnKeyFrameTileDidNotAppearInAssignmentOverride = true;
      break;
    case WNO_KEY_FRAME_DID_NOT_APPEAR_VAL:
      validateSubcommandContext(ctx, WNO_KEY_FRAME_DID_NOT_APPEAR);
      warnKeyFrameTileDidNotAppearInAssignmentOverride = false;
      break;
    case WUSED_TRUE_COLOR_MODE_VAL:
      validateSubcommandContext(ctx, WUSED_TRUE_COLOR_MODE);
      warnUsedTrueColorModeOverride = true;
      break;
    case WNO_USED_TRUE_COLOR_MODE_VAL:
      validateSubcommandContext(ctx, WNO_USED_TRUE_COLOR_MODE);
      warnUsedTrueColorModeOverride = false;
      break;
    case WATTRIBUTE_FORMAT_MISMATCH_VAL:
      validateSubcommandContext(ctx, WATTRIBUTE_FORMAT_MISMATCH);
      warnAttributeFormatMismatchOverride = true;
      break;
    case WNO_ATTRIBUTE_FORMAT_MISMATCH_VAL:
      validateSubcommandContext(ctx, WNO_ATTRIBUTE_FORMAT_MISMATCH);
      warnAttributeFormatMismatchOverride = false;
      break;
    case WMISSING_ATTRIBUTES_CSV_VAL:
      validateSubcommandContext(ctx, WMISSING_ATTRIBUTES_CSV);
      warnMissingAttributesCsvOverride = true;
      break;
    case WNO_MISSING_ATTRIBUTES_CSV_VAL:
      validateSubcommandContext(ctx, WNO_MISSING_ATTRIBUTES_CSV);
      warnMissingAttributesCsvOverride = false;
      break;
    case WUNUSED_ATTRIBUTE_VAL:
      validateSubcommandContext(ctx, WUNUSED_ATTRIBUTE);
      warnUnusedAttributeOverride = true;
      break;
    case WNO_UNUSED_ATTRIBUTE_VAL:
      validateSubcommandContext(ctx, WNO_UNUSED_ATTRIBUTE);
      warnUnusedAttributeOverride = false;
      break;
    case WTRANSPARENCY_COLLAPSE_VAL:
      validateSubcommandContext(ctx, WTRANSPARENCY_COLLAPSE);
      warnTransparencyCollapseOverride = true;
      break;
    case WNO_TRANSPARENCY_COLLAPSE_VAL:
      validateSubcommandContext(ctx, WNO_TRANSPARENCY_COLLAPSE);
      warnTransparencyCollapseOverride = false;
      break;
    case WASSIGN_CONFIG_OVERRIDE_VAL:
      validateSubcommandContext(ctx, WASSIGN_CONFIG_OVERRIDE);
      warnAssignCacheOverride = true;
      break;
    case WNO_ASSIGN_CONFIG_OVERRIDE_VAL:
      validateSubcommandContext(ctx, WNO_ASSIGN_CONFIG_OVERRIDE);
      warnAssignCacheOverride = false;
      break;
    case WINVALID_ASSIGN_CONFIG_CACHE_VAL:
      validateSubcommandContext(ctx, WINVALID_ASSIGN_CONFIG_CACHE);
      warnInvalidAssignCache = true;
      break;
    case WNO_INVALID_ASSIGN_CONFIG_CACHE_VAL:
      validateSubcommandContext(ctx, WNO_INVALID_ASSIGN_CONFIG_CACHE);
      warnInvalidAssignCache = false;
      break;
    case WMISSING_ASSIGN_CONFIG_VAL:
      validateSubcommandContext(ctx, WMISSING_ASSIGN_CONFIG);
      warnMissingAssignCache = true;
      break;
    case WNO_MISSING_ASSIGN_CONFIG_VAL:
      validateSubcommandContext(ctx, WNO_MISSING_ASSIGN_CONFIG);
      warnMissingAssignCache = false;
      break;
    // Decompilation warnings
    case WINVALID_TILE_INDEX_VAL:
      validateSubcommandContext(ctx, WINVALID_TILE_INDEX);
      warnInvalidTileIndex = true;
      break;
    case WNO_INVALID_TILE_INDEX_VAL:
      validateSubcommandContext(ctx, WNO_INVALID_TILE_INDEX);
      warnInvalidTileIndex = false;
      break;

    // Help message upon '-h/--help' goes to stdout
    case HELP_VAL:
      validateSubcommandContext(ctx, HELP);
      if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
        fmt::println("{}", COMPILE_PRIMARY_HELP);
      }
      else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        fmt::println("{}", COMPILE_SECONDARY_HELP);
      }
      else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY) {
        fmt::println("{}", DECOMPILE_PRIMARY_HELP);
      }
      else if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
        fmt::println("{}", DECOMPILE_SECONDARY_HELP);
      }
      else {
        internalerror(
            fmt::format("cli_parser::parseSubcommandOptions unknown subcommand: {}", static_cast<int>(ctx.subcommand)));
      }
      exit(0);
    // Help message on invalid or unknown options goes to stderr and gives error code
    case '?':
    default:
      // TODO 1.0.0 : figure out how to use fatalerror_unrecognizedOption here
      fmt::println(stderr, "Try `{} {} --help' for usage information.", PORYTILES_EXECUTABLE,
                   subcommandString(ctx.subcommand));
      exit(2);
    }
  }

  /*
   * Die immediately if arguments are invalid, otherwise pack them into the context variable
   */
  if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
    if ((argc - optind) != 2) {
      fatalerror(ctx.err, "must specify INPUT-PATH, BEHAVIORS-HEADER args, see `porytiles compile-primary --help'");
    }
  }
  else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    if ((argc - optind) != 3) {
      fatalerror(ctx.err, "must specify INPUT-PATH, PRIMARY-INPUT-PATH, BEHAVIORS-HEADER args, see `porytiles "
                          "compile-secondary --help'");
    }
  }
  else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY) {
    if ((argc - optind) != 2) {
      fatalerror(ctx.err, "must specify INPUT-PATH, BEHAVIORS-HEADER args, see `porytiles decompile-primary --help'");
    }
  }
  else if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
    if ((argc - optind) != 3) {
      fatalerror(ctx.err, "must specify INPUT-PATH, PRIMARY-INPUT-PATH, BEHAVIORS-HEADER args, see `porytiles "
                          "decompile-secondary --help'");
    }
  }
  else {
    internalerror(
        fmt::format("cli_parser::parseSubcommandOptions unknown subcommand: {}", static_cast<int>(ctx.subcommand)));
  }

  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    ctx.compilerSrcPaths.secondarySourcePath = argv[optind++];
  }
  else if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
    ctx.decompilerSrcPaths.secondarySourcePath = argv[optind++];
  }

  if (ctx.subcommand == Subcommand::COMPILE_PRIMARY || ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    ctx.compilerSrcPaths.primarySourcePath = argv[optind++];
    ctx.compilerSrcPaths.metatileBehaviors = argv[optind++];
  }
  else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY || ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
    ctx.decompilerSrcPaths.primarySourcePath = argv[optind++];
    ctx.decompilerSrcPaths.metatileBehaviors = argv[optind++];
  }
  else {
    internalerror(
        fmt::format("cli_parser::parseSubcommandOptions unknown subcommand: {}", static_cast<int>(ctx.subcommand)));
  }

  /*
   * Configure warnings and errors per user specification
   */
  // These general options are overridden by more specific settings
  if (enableAllWarnings) {
    // Enable all warnings
    ctx.err.setAllWarnings(WarningMode::WARN);
  }

  // Specific warn settings take precedence over general warn settings
  // Compilation warnings
  if (warnColorPrecisionLossOverride.has_value()) {
    ctx.err.colorPrecisionLoss = warnColorPrecisionLossOverride.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  if (warnKeyFrameTileDidNotAppearInAssignmentOverride.has_value()) {
    ctx.err.keyFrameTileDidNotAppearInAssignment =
        warnKeyFrameTileDidNotAppearInAssignmentOverride.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  if (warnUsedTrueColorModeOverride.has_value()) {
    ctx.err.usedTrueColorMode = warnUsedTrueColorModeOverride.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  if (warnAttributeFormatMismatchOverride.has_value()) {
    ctx.err.attributeFormatMismatch =
        warnAttributeFormatMismatchOverride.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  if (warnMissingAttributesCsvOverride.has_value()) {
    ctx.err.missingAttributesCsv = warnMissingAttributesCsvOverride.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  if (warnUnusedAttributeOverride.has_value()) {
    ctx.err.unusedAttribute = warnUnusedAttributeOverride.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  if (warnTransparencyCollapseOverride.has_value()) {
    ctx.err.transparencyCollapse = warnTransparencyCollapseOverride.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  if (warnAssignCacheOverride.has_value()) {
    ctx.err.assignCacheOverride = warnAssignCacheOverride.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  if (warnInvalidAssignCache.has_value()) {
    ctx.err.invalidAssignCache = warnInvalidAssignCache.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  if (warnMissingAssignCache.has_value()) {
    ctx.err.missingAssignCache = warnMissingAssignCache.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  // Decompilation warnings
  if (warnInvalidTileIndex.has_value()) {
    ctx.err.invalidTileIndex = warnInvalidTileIndex.value() ? WarningMode::WARN : WarningMode::OFF;
  }

  // If requested, set all enabled warnings to errors
  if (setAllEnabledWarningsToErrors) {
    ctx.err.setAllEnabledWarningsToErrors();
  }

  // Specific err settings take precedence over warns
  // Compilation warnings
  if (errColorPrecisionLossOverride.has_value()) {
    if (errColorPrecisionLossOverride.value()) {
      ctx.err.colorPrecisionLoss = WarningMode::ERR;
    }
    else if ((warnColorPrecisionLossOverride.has_value() && warnColorPrecisionLossOverride.value()) ||
             enableAllWarnings) {
      ctx.err.colorPrecisionLoss = WarningMode::WARN;
    }
    else {
      ctx.err.colorPrecisionLoss = WarningMode::OFF;
    }
  }
  if (errKeyFrameTileDidNotAppearInAssignmentOverride.has_value()) {
    if (errKeyFrameTileDidNotAppearInAssignmentOverride.value()) {
      ctx.err.keyFrameTileDidNotAppearInAssignment = WarningMode::ERR;
    }
    else if ((warnKeyFrameTileDidNotAppearInAssignmentOverride.has_value() &&
              warnKeyFrameTileDidNotAppearInAssignmentOverride.value()) ||
             enableAllWarnings) {
      ctx.err.keyFrameTileDidNotAppearInAssignment = WarningMode::WARN;
    }
    else {
      ctx.err.keyFrameTileDidNotAppearInAssignment = WarningMode::OFF;
    }
  }
  if (errUsedTrueColorModeOverride.has_value()) {
    if (errUsedTrueColorModeOverride.value()) {
      ctx.err.usedTrueColorMode = WarningMode::ERR;
    }
    else if ((warnUsedTrueColorModeOverride.has_value() && warnUsedTrueColorModeOverride.value()) ||
             enableAllWarnings) {
      ctx.err.usedTrueColorMode = WarningMode::WARN;
    }
    else {
      ctx.err.usedTrueColorMode = WarningMode::OFF;
    }
  }
  if (errAttributeFormatMismatchOverride.has_value()) {
    if (errAttributeFormatMismatchOverride.value()) {
      ctx.err.attributeFormatMismatch = WarningMode::ERR;
    }
    else if ((warnAttributeFormatMismatchOverride.has_value() && warnAttributeFormatMismatchOverride.value()) ||
             enableAllWarnings) {
      ctx.err.attributeFormatMismatch = WarningMode::WARN;
    }
    else {
      ctx.err.attributeFormatMismatch = WarningMode::OFF;
    }
  }
  if (errMissingAttributesCsvOverride.has_value()) {
    if (errMissingAttributesCsvOverride.value()) {
      ctx.err.missingAttributesCsv = WarningMode::ERR;
    }
    else if ((warnMissingAttributesCsvOverride.has_value() && warnMissingAttributesCsvOverride.value()) ||
             enableAllWarnings) {
      ctx.err.missingAttributesCsv = WarningMode::WARN;
    }
    else {
      ctx.err.missingAttributesCsv = WarningMode::OFF;
    }
  }
  if (errUnusedAttributeOverride.has_value()) {
    if (errUnusedAttributeOverride.value()) {
      ctx.err.unusedAttribute = WarningMode::ERR;
    }
    else if ((warnUnusedAttributeOverride.has_value() && warnUnusedAttributeOverride.value()) || enableAllWarnings) {
      ctx.err.unusedAttribute = WarningMode::WARN;
    }
    else {
      ctx.err.unusedAttribute = WarningMode::OFF;
    }
  }
  if (errTransparencyCollapseOverride.has_value()) {
    if (errTransparencyCollapseOverride.value()) {
      ctx.err.transparencyCollapse = WarningMode::ERR;
    }
    else if ((warnTransparencyCollapseOverride.has_value() && warnTransparencyCollapseOverride.value()) ||
             enableAllWarnings) {
      ctx.err.transparencyCollapse = WarningMode::WARN;
    }
    else {
      ctx.err.transparencyCollapse = WarningMode::OFF;
    }
  }
  if (errAssignCacheOverride.has_value()) {
    if (errAssignCacheOverride.value()) {
      ctx.err.assignCacheOverride = WarningMode::ERR;
    }
    else if ((warnAssignCacheOverride.has_value() && warnAssignCacheOverride.value()) || enableAllWarnings) {
      ctx.err.assignCacheOverride = WarningMode::WARN;
    }
    else {
      ctx.err.assignCacheOverride = WarningMode::OFF;
    }
  }
  if (errInvalidAssignCache.has_value()) {
    if (errInvalidAssignCache.value()) {
      ctx.err.invalidAssignCache = WarningMode::ERR;
    }
    else if ((warnInvalidAssignCache.has_value() && warnInvalidAssignCache.value()) || enableAllWarnings) {
      ctx.err.invalidAssignCache = WarningMode::WARN;
    }
    else {
      ctx.err.invalidAssignCache = WarningMode::OFF;
    }
  }
  if (errMissingAssignCache.has_value()) {
    if (errMissingAssignCache.value()) {
      ctx.err.missingAssignCache = WarningMode::ERR;
    }
    else if ((warnMissingAssignCache.has_value() && warnMissingAssignCache.value()) || enableAllWarnings) {
      ctx.err.missingAssignCache = WarningMode::WARN;
    }
    else {
      ctx.err.missingAssignCache = WarningMode::OFF;
    }
  }
  // Decompilation warnings
  if (errInvalidTileIndex.has_value()) {
    if (errInvalidTileIndex.value()) {
      ctx.err.invalidTileIndex = WarningMode::ERR;
    }
    else if ((warnInvalidTileIndex.has_value() && warnInvalidTileIndex.value()) || enableAllWarnings) {
      ctx.err.invalidTileIndex = WarningMode::WARN;
    }
    else {
      ctx.err.invalidTileIndex = WarningMode::OFF;
    }
  }

  if (disableAllWarnings) {
    /*
     * Disable all warnings. A single -Wnone specified anywhere on the command line always takes precedence over
     * anything else. To my mind, this is a bit odd. But GCC does it this way, and since we are emulating the way GCC
     * does things, we'll do it too.
     */
    enableAllWarnings = false;
    ctx.err.setAllWarnings(WarningMode::OFF);
  }

  // TODO : should we fail here, or warn the user, or do nothing and just prioritize one over the other?
  // if (ctx.compilerConfig.smartPrune && ctx.compilerConfig.bestBranches > 0) {
  //   fatalerror(ctx.err, fmt::format("found two conflicting configs for `{}' option", BEST_BRANCHES));
  // }

  /*
   * Apply and validate the fieldmap configuration parameters
   */
  if (ctx.targetBaseGame == TargetBaseGame::EMERALD) {
    ctx.fieldmapConfig = FieldmapConfig::pokeemeraldDefaults();
  }
  else if (ctx.targetBaseGame == TargetBaseGame::FIRERED) {
    ctx.fieldmapConfig = FieldmapConfig::pokefireredDefaults();
  }
  else if (ctx.targetBaseGame == TargetBaseGame::RUBY) {
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
  }
  else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    ctx.validateFieldmapParameters(CompilerMode::SECONDARY);
  }
  else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY) {
    ctx.validateFieldmapParameters(DecompilerMode::PRIMARY);
  }
  else if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
    ctx.validateFieldmapParameters(DecompilerMode::SECONDARY);
  }
  else {
    internalerror("cli_parser::parseSubcommandOptions unknown subcommand");
  }

  if (ctx.err.usedTrueColorMode != WarningMode::OFF && ctx.output.paletteMode == TilesOutputPalette::TRUE_COLOR) {
    warn_usedTrueColorMode(ctx.err);
  }

  /*
   * Die if any errors occurred
   */
  if (ctx.err.errCount > 0) {
    if (ctx.subcommand == Subcommand::COMPILE_PRIMARY || ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
      die(ctx.err, "Errors generated during command line parsing. Compilation terminated.");
    }
    die(ctx.err, "Errors generated during command line parsing. Decompilation terminated.");
  }
}
} // namespace porytiles

TEST_CASE("parseCompile should work as expected with all command lines")
{
  // These tests are full of disgusting and evil hacks, avert your gaze
  SUBCASE("Check that the defaults are correct")
  {
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
    porytiles::parseSubcommandOptions(ctx, 3, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::OFF);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::OFF);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::WARN);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::OFF);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::OFF);
  }

  SUBCASE("-Wall should enable everything")
  {
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
    porytiles::parseSubcommandOptions(ctx, 4, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::WARN);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::WARN);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::WARN);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::WARN);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::WARN);
  }

  SUBCASE("-Wall -Werror should enable everything as an error")
  {
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
    porytiles::parseSubcommandOptions(ctx, 5, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::ERR);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::ERR);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::ERR);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::ERR);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::ERR);
  }

  SUBCASE("Should enable a non-default warn, set all to error, then disable the error")
  {
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
    porytiles::parseSubcommandOptions(ctx, 6, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::OFF);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::OFF);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::ERR);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::WARN);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::OFF);
  }

  SUBCASE("Should enable all warnings, then disable one of them")
  {
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
    porytiles::parseSubcommandOptions(ctx, 5, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::OFF);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::WARN);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::WARN);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::WARN);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::WARN);
  }

  SUBCASE("Global warning disable should work, even if a warning was explicitly enabled")
  {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;

    optind = 1;

    char bufCmd[64];
    strcpy(bufCmd, "compile-primary");

    char bufWnone[64];
    strcpy(bufWnone, "-Wnone");

    char bufTrueColor[64];
    strcpy(bufTrueColor, "-Wused-true-color-mode");

    char bufPath[64];
    strcpy(bufPath, "/home/foo/pokeemerald");

    char bufHeader[64];
    strcpy(bufHeader, "/home/foo/metatile_behaviors.h");

    char *const argv[] = {bufCmd, bufWnone, bufTrueColor, bufPath, bufHeader};
    porytiles::parseSubcommandOptions(ctx, 5, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::OFF);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::OFF);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::OFF);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::OFF);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::OFF);
  }
}
