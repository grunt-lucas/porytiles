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
  case Subcommand::COMPILE_FREESTANDING:
    parseCompile(ctx, argc, argv);
    break;
  default:
    throw std::runtime_error{"cli_parser::parseOptions unknown subcommand setting"};
  }
}

template <typename T> static T parseIntegralOption(const std::string &optionName, const char *optarg)
{
  try {
    size_t pos;
    T arg = std::stoi(optarg, &pos);
    if (std::string{optarg}.size() != pos) {
      throw PtException{"option argument was not a valid integral type"};
    }
    return arg;
  }
  catch (const std::exception &e) {
    throw PtException{"invalid argument `" + std::string{optarg} + "' for option `" + optionName + "': " + e.what()};
  }
}

static TilesPngPaletteMode parseTilesPngPaletteMode(const std::string &optionName, const char *optarg)
{
  std::string optargString{optarg};
  if (optargString == "pal0") {
    return TilesPngPaletteMode::PAL0;
  }
  else if (optargString == "true-color") {
    return TilesPngPaletteMode::TRUE_COLOR;
  }
  else if (optargString == "greyscale") {
    return TilesPngPaletteMode::GREYSCALE;
  }
  else {
    throw PtException{"invalid argument `" + optargString + "' for option `" + optionName + "'"};
  }
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
"Overworld tileset compiler for use with the pokeruby, pokeemerald, and\n"
"pokefirered Pokémon Generation 3 decompilation projects. Builds Porymap-ready\n"
"tilesets from an RGBA tilesheet.\n"
"\n"
"Project home page: https://github.com/grunt-lucas/porytiles\n"
"\n"
"\n"
"Usage:\n"
"    porytiles [OPTIONS] COMMAND [OPTIONS] [ARGS ...]\n"
"    porytiles --help\n"
"    porytiles --version\n"
"\n"
"Options:\n" +
HELP_DESCRIPTION + "\n" +
VERBOSE_DESCRIPTION + "\n" +
VERSION_DESCRIPTION + "\n"
"Commands:\n"
"    decompile\n"
"        TODO : implement\n"
"\n"
"    compile-primary\n"
"        Compile a bottom, middle, and top RGBA PNG into a complete primary\n"
"        tileset. All files are generated in-place at the output location.\n"
"\n"
"    compile-secondary\n"
"        Compile a bottom, middle, and top RGBA PNG into a complete secondary\n"
"        tileset. All files are generated in-place at the output location. You\n"
"        must also supply the layers for the paired primary tileset.\n"
"\n"
"    compile-freestanding\n"
"        Compile a single RGBA PNG into a freestanding tileset. This mode will\n"
"        not generate metatiles, and the palettes files will not be generated\n"
"        in-place.\n"
"\n"
"Run `porytiles COMMAND --help' for more information about a command.\n"
"\n"
"To get more help with porytiles, check out the guides at:\n"
"    https://github.com/grunt-lucas/porytiles/wiki/Porytiles-Homepage\n";
// @formatter:on
// clang-format on

static void parseGlobalOptions(PtContext &ctx, int argc, char **argv)
{
  std::ostringstream implodedShorts;
  std::copy(GLOBAL_SHORTS.begin(), GLOBAL_SHORTS.end(), std::ostream_iterator<std::string>(implodedShorts, ""));
  // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
  std::string shortOptions = "+" + implodedShorts.str();
  static struct option longOptions[] = {{HELP_LONG.c_str(), no_argument, nullptr, HELP_SHORT},
                                        {VERBOSE_LONG.c_str(), no_argument, nullptr, VERBOSE_SHORT},
                                        {VERSION_LONG.c_str(), no_argument, nullptr, VERSION_SHORT},
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
const std::string COMPILE_FREESTANDING_COMMAND = "compile-freestanding";
static void parseSubcommand(PtContext &ctx, int argc, char **argv)
{
  if ((argc - optind) == 0) {
    throw PtException{"missing required subcommand, try `porytiles --help' for usage information"};
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
  else if (subcommand == COMPILE_FREESTANDING_COMMAND) {
    ctx.subcommand = Subcommand::COMPILE_FREESTANDING;
  }
  else {
    throw PtException{"unrecognized subcommand: " + subcommand};
  }
}

// ----------------------------
// |    COMPILE-X COMMANDS    |
// ----------------------------
// @formatter:off
// clang-format off
const std::vector<std::string> COMPILE_SHORTS = {std::string{HELP_SHORT}, std::string{OUTPUT_SHORT} + ":"};
const std::string COMPILE_HELP =
"Usage:\n"
"    porytiles " + COMPILE_PRIMARY_COMMAND + " [OPTIONS] PRIMARY-PATH\n"
"    porytiles " + COMPILE_SECONDARY_COMMAND + " [OPTIONS] SECONDARY-PATH PRIMARY-PATH\n"
"    porytiles " + COMPILE_FREESTANDING_COMMAND + " [OPTIONS] TILES\n"
"\n"
"Compile given tilesheet(s) into a tileset. The `primary' and `secondary'\n"
"modes compile a bottom, middle, and top layer tilesheet into a complete\n"
"tileset. That is, they will generate a `metatiles.bin' file along with\n"
"`tiles.png' and the pal files in a `palettes' directory, in-place.\n"
"\n"
"In `freestanding' mode, compile a single tilesheet into a freestanding\n"
"tileset. That is, it will not generate a `metatiles.bin', and the pal files\n"
"are not generated in-place. Input requirements for this mode are accordingly\n"
"relaxed. See the documentation for more details.\n"
"\n"
"Args:\n"
"    <PRIMARY-PATH>\n"
"        Path to a directory containing the source data for a primary set.\n"
"        Must contain at least the `bottom.png', `middle.png' and `top.png'\n"
"        tile sheets. May optionally contain an `anims' folder with animated\n"
"        tile sheets. See the documentation for more details."
"\n"
"    <SECONDARY-PATH>\n"
"        Path to a directory containing the source data for a secondary set.\n"
"        Must contain at least the `bottom.png', `middle.png' and `top.png'\n"
"        tile sheets. May optionally contain an `anims' folder with animated\n"
"        tile sheets. See the documentation for more details."
"\n"
"    <TILES>\n"
"        In `freestanding' mode, a single RGBA PNG tilesheet containing\n"
"        the pixel art to be tile-ized.\n"
"\n"
"Output Options:\n" +
OUTPUT_DESCRIPTION + "\n" +
TILES_PNG_PALETTE_MODE_DESCRIPTION + "\n" +
"Fieldmap Options:\n" +
PRESET_POKEEMERALD_DESCRIPTION + "\n" +
PRESET_POKEFIRERED_DESCRIPTION + "\n" +
PRESET_POKERUBY_DESCRIPTION + "\n" +
NUM_TILES_IN_PRIMARY_DESCRIPTION + "\n" +
NUM_TILES_TOTAL_DESCRIPTION + "\n" +
NUM_METATILES_IN_PRIMARY_DESCRIPTION + "\n" +
NUM_METATILES_TOTAL_DESCRIPTION + "\n" +
NUM_PALETTES_IN_PRIMARY_DESCRIPTION + "\n" +
NUM_PALETTES_TOTAL_DESCRIPTION + "\n" +
"Warnings:\n" + 
WALL_DESCRIPTION + "\n";
// @formatter:on
// clang-format on

static void parseCompile(PtContext &ctx, int argc, char **argv)
{
  std::ostringstream implodedShorts;
  std::copy(COMPILE_SHORTS.begin(), COMPILE_SHORTS.end(), std::ostream_iterator<std::string>(implodedShorts, ""));
  // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
  std::string shortOptions = "+" + implodedShorts.str();
  static struct option longOptions[] = {
      {OUTPUT_LONG.c_str(), required_argument, nullptr, OUTPUT_SHORT},
      {TILES_PNG_PALETTE_MODE_LONG.c_str(), required_argument, nullptr, TILES_PNG_PALETTE_MODE_VAL},
      {PRESET_POKEEMERALD_LONG.c_str(), no_argument, nullptr, PRESET_POKEEMERALD_VAL},
      {PRESET_POKEFIRERED_LONG.c_str(), no_argument, nullptr, PRESET_POKEFIRERED_VAL},
      {PRESET_POKERUBY_LONG.c_str(), no_argument, nullptr, PRESET_POKERUBY_VAL},
      {NUM_TILES_IN_PRIMARY_LONG.c_str(), required_argument, nullptr, NUM_TILES_IN_PRIMARY_VAL},
      {NUM_TILES_TOTAL_LONG.c_str(), required_argument, nullptr, NUM_TILES_TOTAL_VAL},
      {NUM_METATILES_IN_PRIMARY_LONG.c_str(), required_argument, nullptr, NUM_METATILES_IN_PRIMARY_VAL},
      {NUM_METATILES_TOTAL_LONG.c_str(), required_argument, nullptr, NUM_METATILES_TOTAL_VAL},
      {NUM_PALETTES_IN_PRIMARY_LONG.c_str(), required_argument, nullptr, NUM_PALETTES_IN_PRIMARY_VAL},
      {NUM_PALETTES_TOTAL_LONG.c_str(), required_argument, nullptr, NUM_PALETTES_TOTAL_VAL},
      {HELP_LONG.c_str(), no_argument, nullptr, HELP_SHORT},
      {nullptr, no_argument, nullptr, 0}};

  while (true) {
    const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

    if (opt == -1)
      break;

    switch (opt) {
    case OUTPUT_SHORT:
      ctx.output.path = optarg;
      break;
    case TILES_PNG_PALETTE_MODE_VAL:
      ctx.output.paletteMode = parseTilesPngPaletteMode(TILES_PNG_PALETTE_MODE_LONG, optarg);
      break;
    case NUM_TILES_IN_PRIMARY_VAL:
      ctx.fieldmapConfig.numTilesInPrimary = parseIntegralOption<size_t>(NUM_TILES_IN_PRIMARY_LONG, optarg);
      break;
    case NUM_TILES_TOTAL_VAL:
      ctx.fieldmapConfig.numTilesTotal = parseIntegralOption<size_t>(NUM_TILES_TOTAL_LONG, optarg);
      break;
    case NUM_METATILES_IN_PRIMARY_VAL:
      ctx.fieldmapConfig.numMetatilesInPrimary = parseIntegralOption<size_t>(NUM_METATILES_IN_PRIMARY_LONG, optarg);
      break;
    case NUM_METATILES_TOTAL_VAL:
      ctx.fieldmapConfig.numMetatilesTotal = parseIntegralOption<size_t>(NUM_METATILES_TOTAL_LONG, optarg);
      break;
    case NUM_PALETTES_IN_PRIMARY_VAL:
      ctx.fieldmapConfig.numPalettesInPrimary = parseIntegralOption<size_t>(NUM_PALETTES_IN_PRIMARY_LONG, optarg);
      break;
    case NUM_PALETTES_TOTAL_VAL:
      ctx.fieldmapConfig.numPalettesTotal = parseIntegralOption<size_t>(NUM_PALETTES_TOTAL_LONG, optarg);
      break;
    case PRESET_POKEEMERALD_VAL:
      ctx.fieldmapConfig = FieldmapConfig::pokeemeraldDefaults();
      break;
    case PRESET_POKEFIRERED_VAL:
      ctx.fieldmapConfig = FieldmapConfig::pokefireredDefaults();
      break;
    case PRESET_POKERUBY_VAL:
      ctx.fieldmapConfig = FieldmapConfig::pokerubyDefaults();
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

  if (ctx.compilerConfig.mode == CompilerMode::FREESTANDING) {
    if ((argc - optind) != 1) {
      throw PtException{"must specify TILES arg, see `porytiles compile-freestanding --help'"};
    }
    ctx.inputPaths.freestandingTilesheetPath = argv[optind++];
  }
  else {
    if (ctx.compilerConfig.mode == CompilerMode::SECONDARY && (argc - optind) != 2) {
      throw PtException{"must specify SECONDARY-PATH and PRIMARY-PATH args, see `porytiles compile-secondary --help'"};
    }
    else if (ctx.compilerConfig.mode != CompilerMode::SECONDARY && (argc - optind) != 1) {
      throw PtException{"must specify PRIMARY-PATH arg, see `porytiles compile-primary --help'"};
    }

    if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
      ctx.inputPaths.secondaryInputPath = argv[optind++];
    }
    ctx.inputPaths.primaryInputPath = argv[optind++];
  }

  ctx.validate();
}

} // namespace porytiles