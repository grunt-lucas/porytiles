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
static void parseDecompile(PtContext &ctx, int argc, char *const *argv);
static void parseCompile(PtContext &ctx, int argc, char *const *argv);

void parseOptions(PtContext &ctx, int argc, char *const *argv)
{
  parseGlobalOptions(ctx, argc, argv);
  parseSubcommand(ctx, argc, argv);

  switch (ctx.subcommand) {
  case Subcommand::DECOMPILE_PRIMARY:
    parseDecompile(ctx, argc, argv);
    break;
  case Subcommand::DECOMPILE_SECONDARY:
    throw std::runtime_error{"FEATURE : support decompile-secondary command"};
    break;
  case Subcommand::COMPILE_PRIMARY:
  case Subcommand::COMPILE_SECONDARY:
    parseCompile(ctx, argc, argv);
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

// --------------------------------
// |    GLOBAL OPTION PARSING     |
// --------------------------------

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

// ----------------------------
// |    SUBCOMMAND PARSING    |
// ----------------------------

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

// ------------------------------
// |    DECOMPILE-X COMMANDS    |
// ------------------------------
// @formatter:off
// clang-format off
const std::vector<std::string> DECOMPILE_SHORTS = {};
const std::string DECOMPILE_HELP =
"USAGE\n"
"    porytiles " + DECOMPILE_PRIMARY_COMMAND + " [OPTIONS] PRIMARY-PATH BEHAVIORS-HEADER\n"
"    porytiles " + DECOMPILE_SECONDARY_COMMAND + " [OPTIONS] SECONDARY-PATH PARTNER-PRIMARY-PATH BEHAVIORS-HEADER\n"
"\n"
"Decompile a tileset into its constituent RGBA layer PNGs, RGB anim frames, and attributes.csv.\n"
"\n"
"ARGS\n"
"    <PRIMARY-PATH>\n"
"        Path to a directory containing a compiled primary tileset.\n"
"\n"
"    <SECONDARY-PATH>\n"
"        Path to a directory containing a compiled secondary tileset.\n"
"\n"
"    <PARTNER-PRIMARY-PATH>\n"
"        Path to a directory containing a compiled secondary tileset's compiled partner primary\n"
"        set.\n"
"\n"
"    <BEHAVIORS-HEADER>\n"
"        Path to your project's `metatile_behaviors.h' file. This file is likely located in your\n"
"        project's `include/constants' folder.\n"
"\n"
"OPTIONS\n" +
"    For more detailed information about the options below, check out the options pages here:\n" +
"    https://github.com/grunt-lucas/porytiles/wiki#advanced-usage\n" +
"\n" +
"    Driver Options\n" +
OUTPUT_DESC + "\n" +
"    Tileset Decompilation Options\n" +
TARGET_BASE_GAME_DESC + "\n";
// @formatter:on
// clang-format on

static void parseDecompile(PtContext &ctx, int argc, char *const *argv)
{
  std::ostringstream implodedShorts;
  std::copy(DECOMPILE_SHORTS.begin(), DECOMPILE_SHORTS.end(), std::ostream_iterator<std::string>(implodedShorts, ""));
  // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
  std::string shortOptions = "+" + implodedShorts.str();
  struct option longOptions[] = {// Driver options
                                 {OUTPUT.c_str(), required_argument, nullptr, OUTPUT_VAL},
                                 {OUTPUT_SHORT.c_str(), required_argument, nullptr, OUTPUT_VAL},

                                 // Tileset decompilation options
                                 {TARGET_BASE_GAME.c_str(), required_argument, nullptr, TARGET_BASE_GAME_VAL},

                                 // Help
                                 {HELP.c_str(), no_argument, nullptr, HELP_VAL},
                                 {HELP_SHORT.c_str(), no_argument, nullptr, HELP_VAL},

                                 {nullptr, no_argument, nullptr, 0}};

  while (true) {
    const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

    if (opt == -1)
      break;

    switch (opt) {

    // Driver options
    case OUTPUT_VAL:
      ctx.output.path = optarg;
      break;

    // Tileset decompilation options
    case TARGET_BASE_GAME_VAL:
      ctx.targetBaseGame = parseTargetBaseGame(ctx.err, TARGET_BASE_GAME, optarg);
      break;

    // Help message upon '-h/--help' goes to stdout
    case HELP_VAL:
      fmt::println("{}", DECOMPILE_HELP);
      exit(0);
    // Help message on invalid or unknown options goes to stderr and gives error code
    case '?':
    default:
      // FIXME : show correct subcommand here
      fmt::println(stderr, "Try `{} decompile-primary --help' for usage information.", PROGRAM_NAME);
      exit(2);
    }
  }

  /*
   * Die immediately if arguments are invalid, otherwise pack them into the context variable
   */
  if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY && (argc - optind) != 3) {
    fatalerror(ctx.err, "must specify SECONDARY-PATH, PARTNER-PRIMARY-PATH, BEHAVIORS-HEADER args, see "
                        "`porytiles decompile-secondary --help'");
  }
  else if (ctx.subcommand != Subcommand::DECOMPILE_SECONDARY && (argc - optind) != 2) {
    fatalerror(ctx.err, "must specify PRIMARY-PATH, BEHAVIORS-HEADER args, see `porytiles decompile-primary --help'");
  }
  if (ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
    ctx.decompilerSrcPaths.secondarySourcePath = argv[optind++];
  }
  ctx.decompilerSrcPaths.primarySourcePath = argv[optind++];
  ctx.decompilerSrcPaths.metatileBehaviors = argv[optind++];

  /*
   * Apply the target base game
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

  /*
   * Die if any errors occurred
   */
  if (ctx.err.errCount > 0) {
    die(ctx.err, "Errors generated during command line parsing. Decompilation terminated.");
  }
}

// ----------------------------
// |    COMPILE-X COMMANDS    |
// ----------------------------
// @formatter:off
// clang-format off
const std::vector<std::string> COMPILE_SHORTS = {};
const std::string COMPILE_HELP =
"USAGE\n"
"    porytiles " + COMPILE_PRIMARY_COMMAND + " [OPTIONS] PRIMARY-PATH\n"
"    porytiles " + COMPILE_SECONDARY_COMMAND + " [OPTIONS] SECONDARY-PATH PARTNER-PRIMARY-PATH\n"
"\n"
"Compile the tile assets in a given source folder into a Porymap-ready tileset.\n"
"\n"
"ARGS\n"
"    <PRIMARY-PATH>\n"
"        Path to a directory containing the source data for a primary set.\n"
"\n"
"    <SECONDARY-PATH>\n"
"        Path to a directory containing the source data for a secondary set.\n"
"\n"
"    <PARTNER-PRIMARY-PATH>\n"
"        Path to a directory containing the source data for a secondary set's partner primary set.\n"
"        This partner primary set must be a Porytiles-managed tileset.\n"
"\n"
"    Source Directory Format\n"
"        The source directory must conform to the following format. '[]' indicate optional assets.\n"
"            src/\n"
"                bottom.png               # bottom metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                middle.png               # middle metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                top.png                  # top metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                [attributes.csv]         # missing metatile entries will receive default values\n"
"                [metatile_behaviors.h]   # primary sets only, consider symlinking to project metatile_attributes.h\n"
"                [anims/]                 # 'anims' folder is optional\n"
"                    [anim1/]             # animation names can be arbitrary, but must be unique\n"
"                        key.png          # you must specify a key frame PNG for each anim\n"
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
"    Tileset Generation Options\n" +
TARGET_BASE_GAME_DESC + "\n" +
DUAL_LAYER_DESC + "\n" +
TRANSPARENCY_COLOR_DESC + "\n" +
"    Color Assignment Config Options\n" +
ASSIGN_EXPLORE_CUTOFF_DESC + "\n" +
ASSIGN_ALGO_DESC + "\n" +
PRUNE_BRANCHES_DESC + "\n" +
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
static void parseCompile(PtContext &ctx, int argc, char *const *argv)
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

      // Color assignment config options
      {ASSIGN_EXPLORE_CUTOFF.c_str(), required_argument, nullptr, ASSIGN_EXPLORE_CUTOFF_VAL},
      {ASSIGN_ALGO.c_str(), required_argument, nullptr, ASSIGN_ALGO_VAL},
      {PRUNE_BRANCHES.c_str(), required_argument, nullptr, PRUNE_BRANCHES_VAL},

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
      {WMISSING_BEHAVIORS_HEADER.c_str(), no_argument, nullptr, WMISSING_BEHAVIORS_HEADER_VAL},
      {WNO_MISSING_BEHAVIORS_HEADER.c_str(), no_argument, nullptr, WNO_MISSING_BEHAVIORS_HEADER_VAL},
      {WUNUSED_ATTRIBUTE.c_str(), no_argument, nullptr, WUNUSED_ATTRIBUTE_VAL},
      {WNO_UNUSED_ATTRIBUTE.c_str(), no_argument, nullptr, WNO_UNUSED_ATTRIBUTE_VAL},
      {WTRANSPARENCY_COLLAPSE.c_str(), no_argument, nullptr, WTRANSPARENCY_COLLAPSE_VAL},
      {WNO_TRANSPARENCY_COLLAPSE.c_str(), no_argument, nullptr, WNO_TRANSPARENCY_COLLAPSE_VAL},

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

  std::optional<bool> warnMissingBehaviorsHeaderOverride{};
  std::optional<bool> errMissingBehaviorsHeaderOverride{};

  std::optional<bool> warnUnusedAttributeOverride{};
  std::optional<bool> errUnusedAttributeOverride{};

  std::optional<bool> warnTransparencyCollapseOverride{};
  std::optional<bool> errTransparencyCollapseOverride{};

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

    // Color assignment config options
    case ASSIGN_EXPLORE_CUTOFF_VAL:
      cutoffFactor = parseIntegralOption<std::size_t>(ctx.err, ASSIGN_EXPLORE_CUTOFF, optarg);
      // FIXME : error if this factor is too large
      ctx.compilerConfig.exploredNodeCutoff = cutoffFactor * EXPLORATION_CUTOFF_MULTIPLIER;
      break;
    case ASSIGN_ALGO_VAL:
      ctx.compilerConfig.assignAlgorithm = parseAssignAlgorithm(ctx.err, ASSIGN_ALGO, optarg);
      break;
    case PRUNE_BRANCHES_VAL:
      if (std::string{optarg} == "smart") {
        ctx.compilerConfig.smartPrune = true;
      }
      else {
        ctx.compilerConfig.pruneCount = parseIntegralOption<std::size_t>(ctx.err, PRUNE_BRANCHES, optarg);
      }
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
        else if (strcmp(optarg, WARN_MISSING_BEHAVIORS_HEADER) == 0) {
          errMissingBehaviorsHeaderOverride = true;
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
      else if (strcmp(optarg, WARN_MISSING_BEHAVIORS_HEADER) == 0) {
        errMissingBehaviorsHeaderOverride = false;
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
    case WMISSING_BEHAVIORS_HEADER_VAL:
      warnMissingBehaviorsHeaderOverride = true;
      break;
    case WNO_MISSING_BEHAVIORS_HEADER_VAL:
      warnMissingBehaviorsHeaderOverride = false;
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

    // Help message upon '-h/--help' goes to stdout
    case HELP_VAL:
      fmt::println("{}", COMPILE_HELP);
      exit(0);
    // Help message on invalid or unknown options goes to stderr and gives error code
    case '?':
    default:
      // FIXME : show correct subcommand here
      fmt::println(stderr, "Try `{} compile-primary --help' for usage information.", PROGRAM_NAME);
      exit(2);
    }
  }

  /*
   * Die immediately if arguments are invalid, otherwise pack them into the context variable
   */
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY && (argc - optind) != 2) {
    fatalerror(ctx.err, "must specify SECONDARY-PATH and PRIMARY-PATH args, see `porytiles compile-secondary --help'");
  }
  else if (ctx.subcommand != Subcommand::COMPILE_SECONDARY && (argc - optind) != 1) {
    fatalerror(ctx.err, "must specify PRIMARY-PATH arg, see `porytiles compile-primary --help'");
  }
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    ctx.compilerSrcPaths.secondarySourcePath = argv[optind++];
  }
  ctx.compilerSrcPaths.primarySourcePath = argv[optind++];

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
  if (warnMissingBehaviorsHeaderOverride.has_value()) {
    ctx.err.missingBehaviorsHeader = warnMissingBehaviorsHeaderOverride.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  if (warnUnusedAttributeOverride.has_value()) {
    ctx.err.unusedAttribute = warnUnusedAttributeOverride.value() ? WarningMode::WARN : WarningMode::OFF;
  }
  if (warnTransparencyCollapseOverride.has_value()) {
    ctx.err.transparencyCollapse = warnTransparencyCollapseOverride.value() ? WarningMode::WARN : WarningMode::OFF;
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
  if (errMissingBehaviorsHeaderOverride.has_value()) {
    if (errMissingBehaviorsHeaderOverride.value()) {
      ctx.err.missingBehaviorsHeader = WarningMode::ERR;
    }
    else if ((warnMissingBehaviorsHeaderOverride.has_value() && warnMissingBehaviorsHeaderOverride.value()) ||
             enableAllWarnings) {
      ctx.err.missingBehaviorsHeader = WarningMode::WARN;
    }
    else {
      ctx.err.missingBehaviorsHeader = WarningMode::OFF;
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

  if (disableAllWarnings) {
    /*
     * Disable all warnings. A single -Wnone specified anywhere on the command line always takes precedence over
     * anything else. To my mind, this is a bit odd. But GCC does it this way, and since we are emulating the way GCC
     * does things, we'll do it too.
     */
    enableAllWarnings = false;
    ctx.err.setAllWarnings(WarningMode::OFF);
  }

  if (ctx.compilerConfig.smartPrune && ctx.compilerConfig.pruneCount > 0) {
    fatalerror(ctx.err, fmt::format("found two conflicting configs for `{}' option", PRUNE_BRANCHES));
  }

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
    die(ctx.err, "Errors generated during command line parsing. Compilation terminated.");
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

    char *const argv[] = {bufCmd, bufPath};
    porytiles::parseCompile(ctx, 2, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::OFF);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::OFF);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::WARN);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::OFF);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::OFF);
    CHECK(ctx.err.missingBehaviorsHeader == porytiles::WarningMode::OFF);
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

    char *const argv[] = {bufCmd, bufWall, bufPath};
    porytiles::parseCompile(ctx, 3, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::WARN);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::WARN);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::WARN);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::WARN);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::WARN);
    CHECK(ctx.err.missingBehaviorsHeader == porytiles::WarningMode::WARN);
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

    char *const argv[] = {bufCmd, bufWall, bufWerror, bufPath};
    porytiles::parseCompile(ctx, 4, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::ERR);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::ERR);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::ERR);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::ERR);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::ERR);
    CHECK(ctx.err.missingBehaviorsHeader == porytiles::WarningMode::ERR);
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

    char *const argv[] = {bufCmd, bufTrueColor, bufWerror, bufNoError, bufPath};
    porytiles::parseCompile(ctx, 5, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::OFF);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::OFF);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::ERR);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::WARN);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::OFF);
    CHECK(ctx.err.missingBehaviorsHeader == porytiles::WarningMode::OFF);
  }

  SUBCASE("Should enable all warnings, then disable two of them")
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

    char bufNoMissingBehaviorsHeader[64];
    strcpy(bufNoMissingBehaviorsHeader, "-Wno-missing-behaviors-header");

    char bufPath[64];
    strcpy(bufPath, "/home/foo/pokeemerald");

    char *const argv[] = {bufCmd, bufWall, bufNoColorPrecisionLoss, bufNoMissingBehaviorsHeader, bufPath};
    porytiles::parseCompile(ctx, 5, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::OFF);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::WARN);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::WARN);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::WARN);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::WARN);
    CHECK(ctx.err.missingBehaviorsHeader == porytiles::WarningMode::OFF);
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

    char *const argv[] = {bufCmd, bufWnone, bufTrueColor, bufPath};
    porytiles::parseCompile(ctx, 4, argv);

    CHECK(ctx.err.colorPrecisionLoss == porytiles::WarningMode::OFF);
    CHECK(ctx.err.keyFrameTileDidNotAppearInAssignment == porytiles::WarningMode::OFF);
    CHECK(ctx.err.usedTrueColorMode == porytiles::WarningMode::OFF);
    CHECK(ctx.err.attributeFormatMismatch == porytiles::WarningMode::OFF);
    CHECK(ctx.err.missingAttributesCsv == porytiles::WarningMode::OFF);
    CHECK(ctx.err.missingBehaviorsHeader == porytiles::WarningMode::OFF);
  }
}
