#include "cli_parser.h"

#include <getopt.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "cli_options.h"
#include "errors_warnings.h"
#include "logger.h"
#include "program_name.h"
#include "ptexception.h"

namespace porytiles {

static void parseGlobalOptions(PtContext &ctx, int argc, char **argv);
static void parseSubcommand(PtContext &ctx, int argc, char **argv);
static void parseCompile(PtContext &ctx, int argc, char **argv);

void parseOptions(PtContext &ctx, int argc, char **argv)
{
  parseGlobalOptions(ctx, argc, argv);
  parseSubcommand(ctx, argc, argv);

  switch (ctx.subcommand) {
  case Subcommand::DECOMPILE:
    throw std::runtime_error{"TODO : support decompile command"};
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
    size_t pos;
    T arg = std::stoi(optarg, &pos);
    if (std::string{optarg}.size() != pos) {
      // TODO : this is a problem, it throws, re-catches down below, which throws again
      fatalerror_basicprefix(err, fmt::format("option '-{}' argument '{}' was not a valid integral type",
                                              fmt::styled(optionName, fmt::emphasis::bold),
                                              fmt::styled(optarg, fmt::emphasis::bold)));
    }
    return arg;
  }
  catch (const std::exception &e) {
    fatalerror_basicprefix(err, fmt::format("invalid argument '{}' for option '{}': {}",
                                            fmt::styled(optarg, fmt::emphasis::bold),
                                            fmt::styled(optionName, fmt::emphasis::bold), e.what()));
  }
  // unreachable, here for compiler
  throw std::runtime_error("cli_parser::parseIntegralOption reached unreachable code path");
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
    fatalerror_basicprefix(err, fmt::format("invalid argument '{}' for option '{}'",
                                            fmt::styled(optargString, fmt::emphasis::bold),
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
    fatalerror_basicprefix(err, fmt::format("invalid argument '{}' for option '{}'",
                                            fmt::styled(optargString, fmt::emphasis::bold),
                                            fmt::styled(optionName, fmt::emphasis::bold)));
  }
  // unreachable, here for compiler
  throw std::runtime_error("cli_parser::parseTargetBaseGame reached unreachable code path");
}

// --------------------------------
// |    GLOBAL OPTION PARSING     |
// --------------------------------

// @formatter:off
// clang-format off
const std::vector<std::string> GLOBAL_SHORTS = {std::string{HELP_SHORT}, std::string{VERBOSE_SHORT},
                                                std::string{VERSION_SHORT}};
const std::string GLOBAL_HELP =
"porytiles " + VERSION + " " + RELEASE_DATE + "\n"
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
"    decompile\n"
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

static void parseGlobalOptions(PtContext &ctx, int argc, char **argv)
{
  std::ostringstream implodedShorts;
  std::copy(GLOBAL_SHORTS.begin(), GLOBAL_SHORTS.end(), std::ostream_iterator<std::string>(implodedShorts, ""));
  // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
  std::string shortOptions = "+" + implodedShorts.str();
  static struct option longOptions[] = {{HELP.c_str(), no_argument, nullptr, HELP_SHORT},
                                        {VERBOSE.c_str(), no_argument, nullptr, VERBOSE_SHORT},
                                        {VERSION.c_str(), no_argument, nullptr, VERSION_SHORT},
                                        {nullptr, no_argument, nullptr, 0}};

  while (true) {
    const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

    if (opt == -1)
      break;

    switch (opt) {
    case VERBOSE_SHORT:
      ctx.verbose = true;
      break;
    case VERSION_SHORT:
      fmt::println("{} {} {}", PROGRAM_NAME, VERSION, RELEASE_DATE);
      exit(0);

      // Help message upon '-h/--help' goes to stdout
    case HELP_SHORT:
      fmt::println("{}", GLOBAL_HELP);
      exit(0);
      // Help message on invalid or unknown options goes to stderr and gives error code
    case '?':
    default:
      fmt::println(stderr, "{}", GLOBAL_HELP);
      ;
      exit(2);
    }
  }
}

// ----------------------------
// |    SUBCOMMAND PARSING    |
// ----------------------------

const std::string DECOMPILE_COMMAND = "decompile";
const std::string COMPILE_PRIMARY_COMMAND = "compile-primary";
const std::string COMPILE_SECONDARY_COMMAND = "compile-secondary";
static void parseSubcommand(PtContext &ctx, int argc, char **argv)
{
  if ((argc - optind) == 0) {
    fatalerror_basicprefix(ctx.err, "missing required subcommand, try `porytiles --help' for usage information");
  }

  std::string subcommand = argv[optind++];
  if (subcommand == DECOMPILE_COMMAND) {
    ctx.subcommand = Subcommand::DECOMPILE;
  }
  else if (subcommand == COMPILE_PRIMARY_COMMAND) {
    ctx.subcommand = Subcommand::COMPILE_PRIMARY;
  }
  else if (subcommand == COMPILE_SECONDARY_COMMAND) {
    ctx.subcommand = Subcommand::COMPILE_SECONDARY;
  }
  else {
    internalerror("cli_parser::parseSubcommand unrecognized Subcommand");
  }
}

// ----------------------------
// |    COMPILE-X COMMANDS    |
// ----------------------------
// @formatter:off
// clang-format off
const std::vector<std::string> COMPILE_SHORTS = {std::string{HELP_SHORT}, std::string{OUTPUT_SHORT} + ":"};
const std::string COMPILE_HELP =
"USAGE\n"
"    porytiles " + COMPILE_PRIMARY_COMMAND + " [OPTIONS] PRIMARY-PATH\n"
"    porytiles " + COMPILE_SECONDARY_COMMAND + " [OPTIONS] SECONDARY-PATH PARTNER-PRIMARY-PATH\n"
"\n"
"Compile the tile assets in a given input folder into a Porymap-ready tileset.\n"
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
"    Input Directory Format\n"
"        The input directories must conform to the following format. '[]' indicate optional assets.\n"
"            input/\n"
"                bottom.png           # bottom metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                middle.png           # middle metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                top.png              # top metatile layer (RGBA, 8-bit, or 16-bit indexed)\n"
"                attributes.csv       # missing metatile entries will receive default values\n"
"                [anims/]             # 'anims' folder is optional\n"
"                    [anim1/]         # animation names can be arbitrary, but must be unique\n"
"                        key.png      # you must specify a key frame PNG\n"
"                        00.png       # you must specify at least one animation frame\n"
"                        [01.png]     # frames must be named numerically, in order\n"
"                        ...          # you may specify an arbitrary number of additional frames\n"
"                    ...              # you may specify an arbitrary number of additional animations\n"
"\n"
"OPTIONS\n" +
"    Driver Options\n" +
OUTPUT_DESC + "\n" +
"    Tileset Generation Options\n" +
TARGET_BASE_GAME_DESC + "\n" +
TILES_OUTPUT_PAL_DESC + "\n" +
DUAL_LAYER_DESC + "\n" +
"    Fieldmap Override Options\n" +
TILES_PRIMARY_OVERRIDE_DESC + "\n" +
TILES_TOTAL_OVERRIDE_DESC + "\n" +
METATILES_PRIMARY_OVERRIDE_DESC + "\n" +
METATILES_TOTAL_OVERRIDE_DESC + "\n" +
PALS_PRIMARY_OVERRIDE_DESC + "\n" +
PALS_TOTAL_OVERRIDE_DESC + "\n" +
"    Warning Options\n" + 
WALL_DESC + "\n" +
WERROR_DESC + "\n";
// @formatter:on
// clang-format on

static void parseCompile(PtContext &ctx, int argc, char **argv)
{
  std::ostringstream implodedShorts;
  std::copy(COMPILE_SHORTS.begin(), COMPILE_SHORTS.end(), std::ostream_iterator<std::string>(implodedShorts, ""));
  // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
  std::string shortOptions = "+" + implodedShorts.str();
  static struct option longOptions[] = {
      {OUTPUT.c_str(), required_argument, nullptr, OUTPUT_SHORT},
      {TILES_OUTPUT_PAL.c_str(), required_argument, nullptr, TILES_OUTPUT_PAL_VAL},
      {TARGET_BASE_GAME.c_str(), required_argument, nullptr, TARGET_BASE_GAME_VAL},
      {DUAL_LAYER.c_str(), no_argument, nullptr, DUAL_LAYER_VAL},
      {TILES_PRIMARY_OVERRIDE.c_str(), required_argument, nullptr, TILES_PRIMARY_OVERRIDE_VAL},
      {TILES_OVERRIDE_TOTAL.c_str(), required_argument, nullptr, TILES_TOTAL_OVERRIDE_VAL},
      {METATILES_OVERRIDE_PRIMARY.c_str(), required_argument, nullptr, METATILES_PRIMARY_OVERRIDE_VAL},
      {METATILES_OVERRIDE_TOTAL.c_str(), required_argument, nullptr, METATILES_TOTAL_OVERRIDE_VAL},
      {PALS_PRIMARY_OVERRIDE.c_str(), required_argument, nullptr, PALS_PRIMARY_OVERRIDE_VAL},
      {PALS_TOTAL_OVERRIDE.c_str(), required_argument, nullptr, PALS_TOTAL_OVERRIDE_VAL},
      {WALL.c_str(), no_argument, nullptr, WALL_VAL},
      {WERROR.c_str(), no_argument, nullptr, WERROR_VAL},
      {HELP.c_str(), no_argument, nullptr, HELP_SHORT},
      {nullptr, no_argument, nullptr, 0}};

  /*
   * Warning specific variables. We must wait until after all options are processed before we actually enable warnings,
   * since enabling/disabling specific warnings must take precedence over the general -Wall and -Werror flags no matter
   * where in the command line the user specified.
   */
  bool enableAllWarnings = false;
  bool setAllWarningsToErrors = false;

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

  while (true) {
    const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

    if (opt == -1)
      break;

    switch (opt) {
    case OUTPUT_SHORT:
      ctx.output.path = optarg;
      break;
    case TILES_OUTPUT_PAL_VAL:
      ctx.output.paletteMode = parseTilesPngPaletteMode(ctx.err, TILES_OUTPUT_PAL, optarg);
      break;
    case TILES_PRIMARY_OVERRIDE_VAL:
      tilesPrimaryOverridden = true;
      tilesPrimaryOverride = parseIntegralOption<std::size_t>(ctx.err, TILES_PRIMARY_OVERRIDE, optarg);
      break;
    case DUAL_LAYER_VAL:
      ctx.compilerConfig.tripleLayer = false;
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
    case TARGET_BASE_GAME_VAL:
      ctx.targetBaseGame = parseTargetBaseGame(ctx.err, TARGET_BASE_GAME, optarg);
      break;
    case WALL_VAL:
      enableAllWarnings = true;
      break;
    case WERROR_VAL:
      setAllWarningsToErrors = true;
      break;

    // Help message upon '-h/--help' goes to stdout
    case HELP_SHORT:
      fmt::println("{}", COMPILE_HELP);
      exit(0);
    // Help message on invalid or unknown options goes to stderr and gives error code
    case '?':
    default:
      fmt::println(stderr, "{}", COMPILE_HELP);
      exit(2);
    }
  }

  // Die immediately if arguments are invalid, otherwise pack them into the context variable
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY && (argc - optind) != 2) {
    fatalerror_basicprefix(
        ctx.err, "must specify SECONDARY-PATH and PRIMARY-PATH args, see `porytiles compile-secondary --help'");
  }
  else if (ctx.subcommand != Subcommand::COMPILE_SECONDARY && (argc - optind) != 1) {
    fatalerror_basicprefix(ctx.err, "must specify PRIMARY-PATH arg, see `porytiles compile-primary --help'");
  }
  if (ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
    ctx.inputPaths.secondaryInputPath = argv[optind++];
  }
  ctx.inputPaths.primaryInputPath = argv[optind++];

  // Configure warnings per user specification
  if (enableAllWarnings) {
    ctx.err.enableAllWarnings();
  }
  if (setAllWarningsToErrors) {
    ctx.err.setAllEnabledWarningsToErrors();
  }

  // Apply and validate the fieldmap configuration parameters
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

  if (ctx.output.paletteMode == TilesOutputPalette::TRUE_COLOR) {
    // TODO : leave this in until Porymap supports 8bpp input images
    warn_usedTrueColorMode(ctx.err);
  }

  // Die if any errors occurred
  if (ctx.err.errCount > 0) {
    die(ctx.err, "Errors generated during command line parsing. Compilation terminated.");
  }
}

} // namespace porytiles