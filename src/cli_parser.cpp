#include "cli_parser.h"

#include <doctest.h>
#include <getopt.h>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>

#define FMT_HEADER_ONLY
#include <fmt/color.h>

#include "cli_options.h"
#include "errors_warnings.h"
#include "logger.h"
#include "palette_assignment.h"
#include "program_name.h"
#include "ptexception.h"
#include "utilities.h"

namespace porytiles {

static void parseGlobalOptions(PtContext &ctx, int argc, char *const *argv);
static void parseSubcommand(PtContext &ctx, int argc, char *const *argv);
static void parseSubcommandOptions(PtContext &ctx, int argc, char *const *argv);

void parseOptions(PtContext &ctx, int argc, char *const *argv)
{
  parseGlobalOptions(ctx, argc, argv);
  parseSubcommand(ctx, argc, argv);

  switch (ctx.subcommand) {
  case Subcommand::DECOMPILE_PRIMARY:
    parseSubcommandOptions(ctx, argc, argv);
    break;
  case Subcommand::DECOMPILE_SECONDARY:
    throw std::runtime_error{"FEATURE : support decompile-secondary command"};
    break;
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
    fatalerror(err, fmt::format("invalid argument '{}' for option '{}': {}", fmt::styled(optarg, fmt::emphasis::bold),
                                fmt::styled(optionName, fmt::emphasis::bold), e.what()));
  }
  // unreachable, here for compiler
  throw std::runtime_error("cli_parser::parseIntegralOption reached unreachable code path");
}

static RGBA32 parseRgbColor(const ErrorsAndWarnings &err, std::string optionName, const std::string &colorString)
{
  std::vector<std::string> colorComponents = split(colorString, ",");
  if (colorComponents.size() != 3) {
    fatalerror(err, fmt::format("invalid argument '{}' for option '{}': RGB color must have three components",
                                fmt::styled(colorString, fmt::emphasis::bold),
                                fmt::styled(optionName, fmt::emphasis::bold)));
  }
  int red = parseIntegralOption<int>(err, optionName, colorComponents[0].c_str());
  int green = parseIntegralOption<int>(err, optionName, colorComponents[1].c_str());
  int blue = parseIntegralOption<int>(err, optionName, colorComponents[2].c_str());

  if (red < 0 || red > 255) {
    fatalerror(err, fmt::format("invalid red component '{}' for option '{}': range must be 0 <= red <= 255",
                                fmt::styled(red, fmt::emphasis::bold), fmt::styled(optionName, fmt::emphasis::bold)));
  }
  if (green < 0 || green > 255) {
    fatalerror(err, fmt::format("invalid green component '{}' for option '{}': range must be 0 <= green <= 255",
                                fmt::styled(green, fmt::emphasis::bold), fmt::styled(optionName, fmt::emphasis::bold)));
  }
  if (blue < 0 || blue > 255) {
    fatalerror(err, fmt::format("invalid blue component '{}' for option '{}': range must be 0 <= blue <= 255",
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
    fatalerror(err, fmt::format("invalid argument '{}' for option '{}'", fmt::styled(optargString, fmt::emphasis::bold),
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
    fatalerror(err, fmt::format("invalid argument '{}' for option '{}'", fmt::styled(optargString, fmt::emphasis::bold),
                                fmt::styled(optionName, fmt::emphasis::bold)));
  }
  // unreachable, here for compiler
  throw std::runtime_error("cli_parser::parseTargetBaseGame reached unreachable code path");
}

static AssignAlgorithm parseAssignAlgorithm(const ErrorsAndWarnings &err, const std::string &optionName,
                                            const char *optarg)
{
  std::string optargString{optarg};
  if (optargString == "dfs") {
    return AssignAlgorithm::DEPTH_FIRST;
  }
  else if (optargString == "bfs") {
    return AssignAlgorithm::BREADTH_FIRST;
  }
  else {
    fatalerror(err, fmt::format("invalid argument '{}' for option '{}'", fmt::styled(optargString, fmt::emphasis::bold),
                                fmt::styled(optionName, fmt::emphasis::bold)));
  }
  // unreachable, here for compiler
  throw std::runtime_error("cli_parser::parseAssignAlgorithm reached unreachable code path");
}

// @formatter:off
// clang-format off
const std::vector<std::string> GLOBAL_SHORTS = {};
const std::string GLOBAL_HELP =
"porytiles " + VERSION_TAG + " " + RELEASE_DATE + "\n"
"grunt-lucas <grunt.lucas@yahoo.com>\n"
"\n"
"Overworld tileset compiler for use with the pokeruby, pokeemerald, and pokefirered Pokémon\n"
"Generation 3 decompilation projects from pret. Builds Porymap-ready tilesets from RGBA\n"
"(or indexed) tile assets.\n"
"\n"
"Project home page: https://github.com/grunt-lucas/porytiles\n"
"\n"
"\n"
"USAGE\n"
"    porytiles [OPTIONS] COMMAND [OPTIONS] [ARGS ...]\n"
"    porytiles --help\n"
"    porytiles --version\n"
"\n"
"OPTIONS\n" +
HELP_DESC + "\n" +
VERBOSE_DESC + "\n" +
VERSION_DESC + "\n"
"COMMANDS\n"
"    decompile-primary\n"
"        Under construction.\n"
"\n"
"    decompile-secondary\n"
"        Under construction.\n"
"\n"
"    compile-primary\n"
"        Compile a complete primary tileset. All files are generated in-place at the output\n"
"        location.\n"
"\n"
"    compile-secondary\n"
"        Compile a complete secondary tileset. All files are generated in-place at the output\n"
"        location.\n"
"\n"
"Run `porytiles COMMAND --help' for more information about a command.\n"
"\n"
"To get more help with porytiles, check out the guides at:\n"
"    https://github.com/grunt-lucas/porytiles/wiki\n";
// @formatter:on
// clang-format on

static void parseGlobalOptions(PtContext &ctx, int argc, char *const *argv)
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
      fmt::println("{} {} {}", PROGRAM_NAME, VERSION, RELEASE_DATE);
      exit(0);

      // Help message upon '-h/--help' goes to stdout
    case HELP_VAL:
      fmt::println("{}", GLOBAL_HELP);
      exit(0);
      // Help message on invalid or unknown options goes to stderr and gives error code
    case '?':
    default:
      fmt::println(stderr, "Try `{} --help' for usage information.", PROGRAM_NAME);
      exit(2);
    }
  }
}

const std::string DECOMPILE_PRIMARY_COMMAND = "decompile-primary";
const std::string DECOMPILE_SECONDARY_COMMAND = "decompile-secondary";
const std::string COMPILE_PRIMARY_COMMAND = "compile-primary";
const std::string COMPILE_SECONDARY_COMMAND = "compile-secondary";
static void parseSubcommand(PtContext &ctx, int argc, char *const *argv)
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

// @formatter:off
// clang-format off
const std::vector<std::string> COMPILE_SHORTS = {};
const std::string COMPILE_HELP =
"USAGE\n"
"    porytiles " + COMPILE_PRIMARY_COMMAND + " [OPTIONS] SRC-PRIMARY-PATH BEHAVIORS-HEADER\n"
"    porytiles " + COMPILE_SECONDARY_COMMAND + " [OPTIONS] SRC-SECONDARY-PATH SRC-PARTNER-PRIMARY-PATH BEHAVIORS-HEADER\n"
"    porytiles " + DECOMPILE_PRIMARY_COMMAND + " [OPTIONS] BIN-PRIMARY-PATH BEHAVIORS-HEADER\n"
"    porytiles " + DECOMPILE_SECONDARY_COMMAND + " [OPTIONS] BIN-SECONDARY-PATH BIN-PARTNER-PRIMARY-PATH BEHAVIORS-HEADER\n"
"\n"
"Compile the tile assets in a given source folder into a Porymap-ready tileset. Decompile a tileset into its\n"
"constituent RGBA layer PNGs, RGB anim frames, and attributes.csv.\n"
"\n"
"ARGS\n"
"    <SRC-PRIMARY-PATH>\n"
"        Path to a directory containing the source data for a primary set.\n"
"\n"
"    <SRC-SECONDARY-PATH>\n"
"        Path to a directory containing the source data for a secondary set.\n"
"\n"
"    <SRC-PARTNER-PRIMARY-PATH>\n"
"        Path to a directory containing the source data for a secondary set's partner primary set.\n"
"        This partner primary set must be a Porytiles-managed tileset.\n"
"\n"
"    <BIN-PRIMARY-PATH>\n"
"        Path to a directory containing a compiled primary tileset.\n"
"\n"
"    <BIN-SECONDARY-PATH>\n"
"        Path to a directory containing a compiled secondary tileset.\n"
"\n"
"    <BIN-PARTNER-PRIMARY-PATH>\n"
"        Path to a directory containing a compiled secondary tileset's compiled partner primary\n"
"        set.\n"
"\n"
"    <BEHAVIORS-HEADER>\n"
"        Path to your project's `metatile_behaviors.h' file. This file is likely located in your\n"
"        project's `include/constants' folder.\n"
"\n"
"    Source Directory Format\n"
"        The source directory must conform to the following format. '[]' indicate optional assets.\n"
"            src/\n"
"                bottom.png               # bottom metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                middle.png               # middle metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                top.png                  # top metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                [assign.cfg]             # cached configuration for palette assignment algorithm\n"
"                [attributes.csv]         # missing metatile entries will receive default values\n"
"                [anim/]                  # 'anim' folder is optional\n"
"                    [anim1/]             # animation names can be arbitrary, but must be unique\n"
"                        key.png          # you must specify a key frame PNG for each anim\n"
"                        00.png           # you must specify at least one animation frame for each anim\n"
"                        [01.png]         # frames must be named numerically, in order\n"
"                        ...              # you may specify an arbitrary number of additional frames\n"
"                    ...                  # you may specify an arbitrary number of additional animations\n"
"\n"
"    Compiled Directory Format\n"
"        The compiled directory must conform to the following format. '[]' indicate optional assets.\n"
"            bin/\n"
"                metatile_attributes.bin  # binary file containing attributes of each metatile\n"
"                metatiles.bin            # binary file containing metatile entries\n"
"                tiles.png                # indexed png of raw tiles\n"
"                palettes                 # directory of palette files\n"
"                    00.pal               # JASC pal file for palette 0\n"
"                    ...                  # there should be one JASC palette file up to NUM_PALS_TOTAL\n"
"                [anim/]                  # 'anim' folder is optional\n"
"                    [anim1/]             # animation names can be arbitrary, but must be unique\n"
"                        00.png           # you must specify at least one animation frame for each anim\n"
"                        [01.png]         # frames must be named numerically, in order\n"
"                        ...              # you may specify an arbitrary number of additional frames\n"
"                    ...                  # you may specify an arbitrary number of additional animations\n"
"\n"
"OPTIONS\n" +
"    For more detailed information about the options below, check out the options pages here:\n" +
"    https://github.com/grunt-lucas/porytiles/wiki#advanced-usage\n" +
"\n" +
"    Driver Options\n" +
OUTPUT_DESC + "\n" +
TILES_OUTPUT_PAL_DESC + "\n" +
"    Tileset (De)compilation Options\n" +
TARGET_BASE_GAME_DESC + "\n" +
DUAL_LAYER_DESC + "\n" +
TRANSPARENCY_COLOR_DESC + "\n" +
DEFAULT_BEHAVIOR_DESC + "\n" +
DEFAULT_ENCOUNTER_TYPE_DESC + "\n" +
DEFAULT_TERRAIN_TYPE_DESC + "\n" +
"    Color Assignment Config Options\n" +
ASSIGN_EXPLORE_CUTOFF_DESC + "\n" +
ASSIGN_ALGO_DESC + "\n" +
BEST_BRANCHES_DESC + "\n" +
PRIMARY_ASSIGN_EXPLORE_CUTOFF_DESC + "\n" +
PRIMARY_ASSIGN_ALGO_DESC + "\n" +
PRIMARY_BEST_BRANCHES_DESC + "\n" +
CACHE_ASSIGN_CONFIG_DESC + "\n" +
"    Fieldmap Override Options\n" +
TILES_PRIMARY_OVERRIDE_DESC + "\n" +
TILES_TOTAL_OVERRIDE_DESC + "\n" +
METATILES_PRIMARY_OVERRIDE_DESC + "\n" +
METATILES_TOTAL_OVERRIDE_DESC + "\n" +
PALS_PRIMARY_OVERRIDE_DESC + "\n" +
PALS_TOTAL_OVERRIDE_DESC + "\n" +
"    Warning Options\n" +
"        Use these options to enable or disable additional warnings, as well as set specific\n" +
"        warnings as errors. For more information and a full list of available warnings, check:\n" +
"        https://github.com/grunt-lucas/porytiles/wiki/Warnings-and-Errors\n" +
"\n" +
WALL_DESC + "\n" +
WNONE_DESC + "\n" +
W_GENERAL_DESC + "\n" +
WERROR_DESC + "\n";
// @formatter:on
// clang-format on

/*
 * FIXME : the warning parsing system here is a dumpster fire
 */
static void parseSubcommandOptions(PtContext &ctx, int argc, char *const *argv)
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

      // Tileset generation options
      {TARGET_BASE_GAME.c_str(), required_argument, nullptr, TARGET_BASE_GAME_VAL},
      {DUAL_LAYER.c_str(), no_argument, nullptr, DUAL_LAYER_VAL},
      {TRANSPARENCY_COLOR.c_str(), required_argument, nullptr, TRANSPARENCY_COLOR_VAL},
      {DEFAULT_BEHAVIOR.c_str(), required_argument, nullptr, DEFAULT_BEHAVIOR_VAL},
      {DEFAULT_ENCOUNTER_TYPE.c_str(), required_argument, nullptr, DEFAULT_ENCOUNTER_TYPE_VAL},
      {DEFAULT_TERRAIN_TYPE.c_str(), required_argument, nullptr, DEFAULT_TERRAIN_TYPE_VAL},

      // Color assignment config options
      {ASSIGN_EXPLORE_CUTOFF.c_str(), required_argument, nullptr, ASSIGN_EXPLORE_CUTOFF_VAL},
      {ASSIGN_ALGO.c_str(), required_argument, nullptr, ASSIGN_ALGO_VAL},
      {BEST_BRANCHES.c_str(), required_argument, nullptr, BEST_BRANCHES_VAL},
      {PRIMARY_ASSIGN_EXPLORE_CUTOFF.c_str(), required_argument, nullptr, PRIMARY_ASSIGN_EXPLORE_CUTOFF_VAL},
      {PRIMARY_ASSIGN_ALGO.c_str(), required_argument, nullptr, PRIMARY_ASSIGN_ALGO_VAL},
      {PRIMARY_BEST_BRANCHES.c_str(), required_argument, nullptr, PRIMARY_BEST_BRANCHES_VAL},
      {CACHE_ASSIGN_CONFIG.c_str(), no_argument, nullptr, CACHE_ASSIGN_CONFIG_VAL},

      // Fieldmap override options
      {TILES_PRIMARY_OVERRIDE.c_str(), required_argument, nullptr, TILES_PRIMARY_OVERRIDE_VAL},
      {TILES_OVERRIDE_TOTAL.c_str(), required_argument, nullptr, TILES_TOTAL_OVERRIDE_VAL},
      {METATILES_OVERRIDE_PRIMARY.c_str(), required_argument, nullptr, METATILES_PRIMARY_OVERRIDE_VAL},
      {METATILES_OVERRIDE_TOTAL.c_str(), required_argument, nullptr, METATILES_TOTAL_OVERRIDE_VAL},
      {PALS_PRIMARY_OVERRIDE.c_str(), required_argument, nullptr, PALS_PRIMARY_OVERRIDE_VAL},
      {PALS_TOTAL_OVERRIDE.c_str(), required_argument, nullptr, PALS_TOTAL_OVERRIDE_VAL},

      // Warning and error options
      {WALL.c_str(), no_argument, nullptr, WALL_VAL},
      {WNONE.c_str(), no_argument, nullptr, WNONE_VAL},
      {WNONE_SHORT.c_str(), no_argument, nullptr, WNONE_VAL},
      {WERROR.c_str(), optional_argument, nullptr, WERROR_VAL},
      {WNO_ERROR.c_str(), required_argument, nullptr, WNO_ERROR_VAL},

      // Specific warnings
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

  std::optional<bool> warnColorPrecisionLossOverride{};
  std::optional<bool> errColorPrecisionLossOverride{};

  std::optional<bool> warnKeyFrameTileDidNotAppearInAssignmentOverride{};
  std::optional<bool> errKeyFrameTileDidNotAppearInAssignmentOverride{};

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

  std::optional<bool> warnAssignConfigOverride{true};
  std::optional<bool> errAssignConfigOverride{};

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

  std::size_t cutoffFactor;

  while (true) {
    const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

    if (opt == -1)
      break;

    switch (opt) {

    // Driver options
    case OUTPUT_VAL:
      ctx.output.path = optarg;
      break;
    case TILES_OUTPUT_PAL_VAL:
      ctx.output.paletteMode = parseTilesPngPaletteMode(ctx.err, TILES_OUTPUT_PAL, optarg);
      break;

    // Tileset generation options
    case TARGET_BASE_GAME_VAL:
      ctx.targetBaseGame = parseTargetBaseGame(ctx.err, TARGET_BASE_GAME, optarg);
      break;
    case DUAL_LAYER_VAL:
      ctx.compilerConfig.tripleLayer = false;
      break;
    case TRANSPARENCY_COLOR_VAL:
      ctx.compilerConfig.transparencyColor = parseRgbColor(ctx.err, TRANSPARENCY_COLOR, optarg);
      break;
    case DEFAULT_BEHAVIOR_VAL:
      ctx.compilerConfig.defaultBehavior = std::string{optarg};
      break;
    case DEFAULT_ENCOUNTER_TYPE_VAL:
      ctx.compilerConfig.defaultEncounterType = std::string{optarg};
      break;
    case DEFAULT_TERRAIN_TYPE_VAL:
      ctx.compilerConfig.defaultTerrainType = std::string{optarg};
      break;

    // Color assignment config options
    case ASSIGN_EXPLORE_CUTOFF_VAL:
      ctx.compilerConfig.providedAssignConfigOverride = true;
      cutoffFactor = parseIntegralOption<std::size_t>(ctx.err, ASSIGN_EXPLORE_CUTOFF, optarg);
      if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
        ctx.compilerConfig.primaryExploredNodeCutoff = cutoffFactor * EXPLORATION_CUTOFF_MULTIPLIER;
        if (ctx.compilerConfig.primaryExploredNodeCutoff > EXPLORATION_MAX_CUTOFF) {
          fatalerror(ctx.err, fmt::format("option '{}' argument cannot be > 100",
                                          fmt::styled(ASSIGN_EXPLORE_CUTOFF, fmt::emphasis::bold)));
        }
      }
      else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        ctx.compilerConfig.secondaryExploredNodeCutoff = cutoffFactor * EXPLORATION_CUTOFF_MULTIPLIER;
        if (ctx.compilerConfig.secondaryExploredNodeCutoff > EXPLORATION_MAX_CUTOFF) {
          fatalerror(ctx.err, fmt::format("option '{}' argument cannot be > 100",
                                          fmt::styled(ASSIGN_EXPLORE_CUTOFF, fmt::emphasis::bold)));
        }
      }
      break;
    case ASSIGN_ALGO_VAL:
      ctx.compilerConfig.providedAssignConfigOverride = true;
      if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
        ctx.compilerConfig.primaryAssignAlgorithm = parseAssignAlgorithm(ctx.err, ASSIGN_ALGO, optarg);
      }
      else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        ctx.compilerConfig.secondaryAssignAlgorithm = parseAssignAlgorithm(ctx.err, ASSIGN_ALGO, optarg);
      }
      break;
    case BEST_BRANCHES_VAL:
      ctx.compilerConfig.providedAssignConfigOverride = true;
      if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
        if (std::string{optarg} == "smart") {
          ctx.compilerConfig.primarySmartPrune = true;
        }
        else {
          ctx.compilerConfig.primaryBestBranches = parseIntegralOption<std::size_t>(ctx.err, BEST_BRANCHES, optarg);
          if (ctx.compilerConfig.primaryBestBranches == 0) {
            fatalerror(ctx.err, fmt::format("option '{}' argument cannot be 0",
                                            fmt::styled(BEST_BRANCHES, fmt::emphasis::bold)));
          }
        }
      }
      else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        if (std::string{optarg} == "smart") {
          ctx.compilerConfig.secondarySmartPrune = true;
        }
        else {
          ctx.compilerConfig.secondaryBestBranches = parseIntegralOption<std::size_t>(ctx.err, BEST_BRANCHES, optarg);
          if (ctx.compilerConfig.secondaryBestBranches == 0) {
            fatalerror(ctx.err, fmt::format("option '{}' argument cannot be 0",
                                            fmt::styled(BEST_BRANCHES, fmt::emphasis::bold)));
          }
        }
      }
      break;
    case PRIMARY_ASSIGN_EXPLORE_CUTOFF_VAL:
      ctx.compilerConfig.providedPrimaryAssignConfigOverride = true;
      cutoffFactor = parseIntegralOption<std::size_t>(ctx.err, PRIMARY_ASSIGN_EXPLORE_CUTOFF, optarg);
      if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        ctx.compilerConfig.primaryExploredNodeCutoff = cutoffFactor * EXPLORATION_CUTOFF_MULTIPLIER;
        if (ctx.compilerConfig.primaryExploredNodeCutoff > EXPLORATION_MAX_CUTOFF) {
          fatalerror(ctx.err, fmt::format("option '{}' argument cannot be > 100",
                                          fmt::styled(PRIMARY_ASSIGN_EXPLORE_CUTOFF, fmt::emphasis::bold)));
        }
      }
      break;
    case PRIMARY_ASSIGN_ALGO_VAL:
      ctx.compilerConfig.providedPrimaryAssignConfigOverride = true;
      if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        ctx.compilerConfig.primaryAssignAlgorithm = parseAssignAlgorithm(ctx.err, PRIMARY_ASSIGN_ALGO, optarg);
      }
      break;
    case PRIMARY_BEST_BRANCHES_VAL:
      ctx.compilerConfig.providedPrimaryAssignConfigOverride = true;
      if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        if (std::string{optarg} == "smart") {
          ctx.compilerConfig.primarySmartPrune = true;
        }
        else {
          ctx.compilerConfig.primaryBestBranches =
              parseIntegralOption<std::size_t>(ctx.err, PRIMARY_BEST_BRANCHES, optarg);
          if (ctx.compilerConfig.primaryBestBranches == 0) {
            fatalerror(ctx.err, fmt::format("option '{}' argument cannot be 0",
                                            fmt::styled(PRIMARY_BEST_BRANCHES, fmt::emphasis::bold)));
          }
        }
      }
      break;
    case CACHE_ASSIGN_CONFIG_VAL:
      ctx.compilerConfig.cacheAssignConfig = true;
      break;

    // Fieldmap override options
    case TILES_PRIMARY_OVERRIDE_VAL:
      tilesPrimaryOverridden = true;
      tilesPrimaryOverride = parseIntegralOption<std::size_t>(ctx.err, TILES_PRIMARY_OVERRIDE, optarg);
      break;
    case TILES_TOTAL_OVERRIDE_VAL:
      tilesTotalOverridden = true;
      tilesTotalOverride = parseIntegralOption<std::size_t>(ctx.err, TILES_OVERRIDE_TOTAL, optarg);
      break;
    case METATILES_PRIMARY_OVERRIDE_VAL:
      metatilesPrimaryOverridden = true;
      metatilesPrimaryOverride = parseIntegralOption<std::size_t>(ctx.err, METATILES_OVERRIDE_PRIMARY, optarg);
      break;
    case METATILES_TOTAL_OVERRIDE_VAL:
      metatilesTotalOverridden = true;
      metatilesTotalOverride = parseIntegralOption<std::size_t>(ctx.err, METATILES_OVERRIDE_TOTAL, optarg);
      break;
    case PALS_PRIMARY_OVERRIDE_VAL:
      palettesPrimaryOverridden = true;
      palettesPrimaryOverride = parseIntegralOption<std::size_t>(ctx.err, PALS_PRIMARY_OVERRIDE, optarg);
      break;
    case PALS_TOTAL_OVERRIDE_VAL:
      palettesTotalOverridden = true;
      palettesTotalOverride = parseIntegralOption<std::size_t>(ctx.err, PALS_TOTAL_OVERRIDE, optarg);
      break;

    // Warning and error options
    case WALL_VAL:
      enableAllWarnings = true;
      break;
    case WNONE_VAL:
      disableAllWarnings = true;
      break;
    case WERROR_VAL:
      if (optarg == NULL) {
        setAllEnabledWarningsToErrors = true;
      }
      else {
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
        else {
          fatalerror(ctx.err, fmt::format("invalid argument '{}' for option '{}'",
                                          fmt::styled(std::string{optarg}, fmt::emphasis::bold),
                                          fmt::styled(WERROR, fmt::emphasis::bold)));
        }
      }
      break;
    case WNO_ERROR_VAL:
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
      else {
        fatalerror(ctx.err, fmt::format("invalid argument '{}' for option '{}'",
                                        fmt::styled(std::string{optarg}, fmt::emphasis::bold),
                                        fmt::styled(WERROR, fmt::emphasis::bold)));
      }
      break;

    // Specific warnings
    case WCOLOR_PRECISION_LOSS_VAL:
      warnColorPrecisionLossOverride = true;
      break;
    case WNO_COLOR_PRECISION_LOSS_VAL:
      warnColorPrecisionLossOverride = false;
      break;
    case WKEY_FRAME_DID_NOT_APPEAR_VAL:
      warnKeyFrameTileDidNotAppearInAssignmentOverride = true;
      break;
    case WNO_KEY_FRAME_DID_NOT_APPEAR_VAL:
      warnKeyFrameTileDidNotAppearInAssignmentOverride = false;
      break;
    case WUSED_TRUE_COLOR_MODE_VAL:
      warnUsedTrueColorModeOverride = true;
      break;
    case WNO_USED_TRUE_COLOR_MODE_VAL:
      warnUsedTrueColorModeOverride = false;
      break;
    case WATTRIBUTE_FORMAT_MISMATCH_VAL:
      warnAttributeFormatMismatchOverride = true;
      break;
    case WNO_ATTRIBUTE_FORMAT_MISMATCH_VAL:
      warnAttributeFormatMismatchOverride = false;
      break;
    case WMISSING_ATTRIBUTES_CSV_VAL:
      warnMissingAttributesCsvOverride = true;
      break;
    case WNO_MISSING_ATTRIBUTES_CSV_VAL:
      warnMissingAttributesCsvOverride = false;
      break;
    case WUNUSED_ATTRIBUTE_VAL:
      warnUnusedAttributeOverride = true;
      break;
    case WNO_UNUSED_ATTRIBUTE_VAL:
      warnUnusedAttributeOverride = false;
      break;
    case WTRANSPARENCY_COLLAPSE_VAL:
      warnTransparencyCollapseOverride = true;
      break;
    case WNO_TRANSPARENCY_COLLAPSE_VAL:
      warnTransparencyCollapseOverride = false;
      break;
    case WASSIGN_CONFIG_OVERRIDE_VAL:
      warnAssignConfigOverride = true;
      break;
    case WNO_ASSIGN_CONFIG_OVERRIDE_VAL:
      warnAssignConfigOverride = false;
      break;

    // Help message upon '-h/--help' goes to stdout
    case HELP_VAL:
      fmt::println("{}", COMPILE_HELP);
      exit(0);
    // Help message on invalid or unknown options goes to stderr and gives error code
    case '?':
    default:
      if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
        fmt::println(stderr, "Try `{} compile-primary --help' for usage information.", PROGRAM_NAME);
      }
      else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
        fmt::println(stderr, "Try `{} compile-secondary --help' for usage information.", PROGRAM_NAME);
      }
      else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY) {
        fmt::println(stderr, "Try `{} decompile-primary --help' for usage information.", PROGRAM_NAME);
      }
      else if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
        fmt::println(stderr, "Try `{} decompile-secondary --help' for usage information.", PROGRAM_NAME);
      }
      else {
        internalerror(
            fmt::format("cli_parser::parseSubcommandOptions unknown subcommand: {}", static_cast<int>(ctx.subcommand)));
      }
      exit(2);
    }
  }

  /*
   * Die immediately if arguments are invalid, otherwise pack them into the context variable
   */
  if (ctx.subcommand == Subcommand::COMPILE_PRIMARY) {
    if ((argc - optind) != 2) {
      fatalerror(ctx.err,
                 "must specify SRC-PRIMARY-PATH, BEHAVIORS-HEADER args, see `porytiles compile-primary --help'");
    }
  }
  else if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    if ((argc - optind) != 3) {
      fatalerror(ctx.err,
                 "must specify SRC-SECONDARY-PATH, SRC-PARTNER-PRIMARY-PATH, BEHAVIORS-HEADER args, see `porytiles "
                 "compile-secondary --help'");
    }
  }
  else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY) {
    if ((argc - optind) != 2) {
      fatalerror(ctx.err,
                 "must specify BIN-PRIMARY-PATH, BEHAVIORS-HEADER args, see `porytiles decompile-primary --help'");
    }
  }
  else if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
    if ((argc - optind) != 3) {
      fatalerror(ctx.err, "must specify BIN-SECONDARY-PATH, BIN-PARTNER-PRIMARY-PATH, BEHAVIORS-HEADER args, see "
                          "`porytiles decompile-secondary --help'");
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
  if (warnAssignConfigOverride.has_value()) {
    ctx.err.assignConfigOverride = warnAssignConfigOverride.value() ? WarningMode::WARN : WarningMode::OFF;
  }

  // If requested, set all enabled warnings to errors
  if (setAllEnabledWarningsToErrors) {
    ctx.err.setAllEnabledWarningsToErrors();
  }

  // Specific err settings take precedence over warns
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
  if (errAssignConfigOverride.has_value()) {
    if (errAssignConfigOverride.value()) {
      ctx.err.assignConfigOverride = WarningMode::ERR;
    }
    else if ((warnAssignConfigOverride.has_value() && warnAssignConfigOverride.value()) || enableAllWarnings) {
      ctx.err.assignConfigOverride = WarningMode::WARN;
    }
    else {
      ctx.err.assignConfigOverride = WarningMode::OFF;
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
  ctx.validateFieldmapParameters();

  if (ctx.err.usedTrueColorMode != WarningMode::OFF && ctx.output.paletteMode == TilesOutputPalette::TRUE_COLOR) {
    // TODO : change this once Porymap supports 8bpp input images
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
    porytiles::PtContext ctx{};
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
    porytiles::PtContext ctx{};
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
    porytiles::PtContext ctx{};
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
    porytiles::PtContext ctx{};
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
    porytiles::PtContext ctx{};
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
    porytiles::PtContext ctx{};
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
