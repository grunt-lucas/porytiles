#include "cli_parser.h"

#include <getopt.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include "cli_options.h"
#include "errors.h"
#include "program_name.h"
#include "ptexception.h"

namespace porytiles {

static void parseGlobalOptions(PtContext &ctx, int argc, char **argv);
static void parseSubcommand(PtContext &ctx, int argc, char **argv);
static void parseCompileRaw(PtContext &ctx, int argc, char **argv);
static void parseCompile(PtContext &ctx, int argc, char **argv);

void parseOptions(PtContext &ctx, int argc, char **argv)
{
  parseGlobalOptions(ctx, argc, argv);
  parseSubcommand(ctx, argc, argv);

  switch (ctx.subcommand) {
  case COMPILE:
    parseCompile(ctx, argc, argv);
    break;
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
"    compile-raw\n"
"        Compile one RGBA PNG layer into palettes and tiles. Won't generate\n"
"        a `metatiles.bin' file.\n"
"\n"
"    compile\n"
"        Compile three RGBA PNG layers into a complete tileset.\n"
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

const std::string COMPILE_RAW_COMMAND = "compile-raw";
const std::string COMPILE_COMMAND = "compile";
static void parseSubcommand(PtContext &ctx, int argc, char **argv)
{
  if ((argc - optind) == 0) {
    throw PtException{"missing required subcommand, try `porytiles --help' for usage information"};
  }

  std::string subcommand = argv[optind++];
  if (subcommand == COMPILE_COMMAND) {
    ctx.subcommand = Subcommand::COMPILE;
  }
  else {
    throw PtException{"unrecognized subcommand: " + subcommand};
  }
}

// -----------------------------
// |    COMPILE-RAW COMMAND    |
// -----------------------------
// TODO : convert this logic to `compile --freestanding' mode
// @formatter:off
// clang-format off
const std::vector<std::string> COMPILE_RAW_SHORTS = {std::string{HELP_SHORT}, std::string{OUTPUT_SHORT} + ":"};
const std::string COMPILE_RAW_HELP =
"Usage:\n"
"    porytiles " + COMPILE_RAW_COMMAND + " [OPTIONS] TILES\n"
"    porytiles " + COMPILE_RAW_COMMAND + " [OPTIONS] --secondary TILES TILES_PRIMARY\n"
"\n"
"Compile one RGBA PNG, i.e. a single tilesheet with no layer information. This\n"
"command will generate pal files and a `tiles.png', but will not generate a\n"
"`metatiles.bin' file.\n"
"\n"
"Args:\n"
"    <TILES>\n"
"        An RGBA PNG tilesheet containing raw pixel art to be tile-ized.\n"
"\n"
"    <TILES_PRIMARY>\n"
"        In `--secondary' mode, an RGBA PNG tilesheet containing the raw\n"
"        pixel art for the corresponding primary set.\n"
"\n"
"Options:\n" +
OUTPUT_DESCRIPTION + "\n" +
TILES_PNG_PALETTE_MODE_DESCRIPTION + "\n" +
SECONDARY_DESCRIPTION + "\n" +
"Per-Game Fieldmap Presets:\n" +
PRESET_POKEEMERALD_DESCRIPTION + "\n" +
PRESET_POKEFIRERED_DESCRIPTION + "\n" +
PRESET_POKERUBY_DESCRIPTION + "\n" +
"Individual Fieldmap Options:\n" +
NUM_TILES_IN_PRIMARY_DESCRIPTION + "\n" +
NUM_TILES_TOTAL_DESCRIPTION + "\n" +
NUM_PALETTES_IN_PRIMARY_DESCRIPTION + "\n" +
NUM_PALETTES_TOTAL_DESCRIPTION + "\n";
// @formatter:on
// clang-format on

// static void parseCompileRaw(PtContext& ctx, int argc, char **argv)
// {
//   std::ostringstream implodedShorts;
//   std::copy(COMPILE_RAW_SHORTS.begin(), COMPILE_RAW_SHORTS.end(),
//             std::ostream_iterator<std::string>(implodedShorts, ""));
//   // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
//   std::string shortOptions = "+" + implodedShorts.str();
//   static struct option longOptions[] = {
//       {OUTPUT_LONG.c_str(), required_argument, nullptr, OUTPUT_SHORT},
//       {TILES_PNG_PALETTE_MODE_LONG.c_str(), required_argument, nullptr, TILES_PNG_PALETTE_MODE_VAL},
//       {SECONDARY_LONG.c_str(), no_argument, nullptr, SECONDARY_VAL},
//       {PRESET_POKEEMERALD_LONG.c_str(), no_argument, nullptr, PRESET_POKEEMERALD_VAL},
//       {PRESET_POKEFIRERED_LONG.c_str(), no_argument, nullptr, PRESET_POKEFIRERED_VAL},
//       {PRESET_POKERUBY_LONG.c_str(), no_argument, nullptr, PRESET_POKERUBY_VAL},
//       {NUM_TILES_IN_PRIMARY_LONG.c_str(), required_argument, nullptr, NUM_TILES_IN_PRIMARY_VAL},
//       {NUM_TILES_TOTAL_LONG.c_str(), required_argument, nullptr, NUM_TILES_TOTAL_VAL},
//       {NUM_PALETTES_IN_PRIMARY_LONG.c_str(), required_argument, nullptr, NUM_PALETTES_IN_PRIMARY_VAL},
//       {NUM_PALETTES_TOTAL_LONG.c_str(), required_argument, nullptr, NUM_PALETTES_TOTAL_VAL},
//       {HELP_LONG.c_str(), no_argument, nullptr, HELP_SHORT},
//       {nullptr, no_argument, nullptr, 0}};

//   while (true) {
//     const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

//     if (opt == -1)
//       break;

//     switch (opt) {
//     case OUTPUT_SHORT:
//       config.outputPath = optarg;
//       break;
//     case TILES_PNG_PALETTE_MODE_VAL:
//       config.tilesPngPaletteMode = parseTilesPngPaletteMode(TILES_PNG_PALETTE_MODE_LONG, optarg);
//       break;
//     case SECONDARY_VAL:
//       config.secondary = true;
//       break;
//     case NUM_PALETTES_IN_PRIMARY_VAL:
//       config.numPalettesInPrimary = parseIntegralOption<size_t>(NUM_PALETTES_IN_PRIMARY_LONG, optarg);
//       break;
//     case NUM_PALETTES_TOTAL_VAL:
//       config.numPalettesTotal = parseIntegralOption<size_t>(NUM_PALETTES_TOTAL_LONG, optarg);
//       break;
//     case NUM_TILES_IN_PRIMARY_VAL:
//       config.numTilesInPrimary = parseIntegralOption<size_t>(NUM_TILES_IN_PRIMARY_LONG, optarg);
//       break;
//     case NUM_TILES_TOTAL_VAL:
//       config.numTilesTotal = parseIntegralOption<size_t>(NUM_TILES_TOTAL_LONG, optarg);
//       break;
//     case PRESET_POKEEMERALD_VAL:
//       setPokeemeraldDefaultTilesetParams(config);
//       break;
//     case PRESET_POKEFIRERED_VAL:
//       setPokefireredDefaultTilesetParams(config);
//       break;
//     case PRESET_POKERUBY_VAL:
//       setPokerubyDefaultTilesetParams(config);
//       break;

//     // Help message upon '-h/--help' goes to stdout
//     case HELP_SHORT:
//       fmt::println("{}", COMPILE_RAW_HELP);
//       exit(0);
//     // Help message on invalid or unknown options goes to stderr and gives error code
//     case '?':
//     default:
//       fmt::println(stderr, "{}", COMPILE_RAW_HELP);
//       exit(2);
//     }
//   }

//   if (config.secondary && (argc - optind) != 2) {
//     throw PtException{"must specify TILES and TILES_PRIMARY args, see `porytiles compile-raw --help'"};
//   }
//   else if ((argc - optind) != 1) {
//     throw PtException{"must specify one TILES arg, see `porytiles compile-raw --help'"};
//   }

//   if (config.secondary) {
//     config.rawSecondaryTilesheetPath = argv[optind++];
//   }
//   config.rawPrimaryTilesheetPath = argv[optind++];

//   config.validate();

//   throw PtException{"TODO : support `compile-raw' correctly"};
// }

// -------------------------
// |    COMPILE COMMAND    |
// -------------------------
// @formatter:off
// clang-format off
const std::vector<std::string> COMPILE_SHORTS = {std::string{HELP_SHORT}, std::string{OUTPUT_SHORT} + ":"};
const std::string COMPILE_HELP =
"Usage:\n"
"    porytiles " + COMPILE_COMMAND + " [OPTIONS] BOTTOM MIDDLE TOP\n"
"    porytiles " + COMPILE_COMMAND + " [OPTIONS] --secondary BOTTOM MIDDLE TOP BOTTOM-PRIMARY MIDDLE-PRIMARY TOP-PRIMARY\n"
"\n"
"Compile a bottom, middle, and top layer tilesheet into a complete tileset.\n"
"This command will generate a `metatiles.bin' file along with `tiles.png' and\n"
"the pal files.\n"
"\n"
"Args:\n"
"    <BOTTOM>\n"
"        An RGBA PNG tilesheet containing the bottom metatile layer.\n"
"\n"
"    <MIDDLE>\n"
"        An RGBA PNG tilesheet containing the middle metatile layer.\n"
"\n"
"    <TOP>\n"
"        An RGBA PNG tilesheet containing the top metatile layer.\n"
"\n"
"    <BOTTOM-PRIMARY>\n"
"        In `--secondary' mode, an RGBA PNG tilesheet containing the bottom\n"
"        metatile layer for the corresponding primary set.\n"
"\n"
"    <MIDDLE-PRIMARY>\n"
"        In `--secondary' mode, an RGBA PNG tilesheet containing the middle\n"
"        metatile layer for the corresponding primary set.\n"
"\n"
"    <TOP-PRIMARY>\n"
"        In `--secondary' mode, an RGBA PNG tilesheet containing the top\n"
"        metatile layer for the corresponding primary set.\n"
"\n"
"Options:\n" +
OUTPUT_DESCRIPTION + "\n" +
TILES_PNG_PALETTE_MODE_DESCRIPTION + "\n" +
SECONDARY_DESCRIPTION + "\n" +
"Per-Game Fieldmap Presets:\n" +
PRESET_POKEEMERALD_DESCRIPTION + "\n" +
PRESET_POKEFIRERED_DESCRIPTION + "\n" +
PRESET_POKERUBY_DESCRIPTION + "\n" +
"Individual Fieldmap Options:\n" +
NUM_TILES_IN_PRIMARY_DESCRIPTION + "\n" +
NUM_TILES_TOTAL_DESCRIPTION + "\n" +
NUM_METATILES_IN_PRIMARY_DESCRIPTION + "\n" +
NUM_METATILES_TOTAL_DESCRIPTION + "\n" +
NUM_PALETTES_IN_PRIMARY_DESCRIPTION + "\n" +
NUM_PALETTES_TOTAL_DESCRIPTION + "\n";
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
      {SECONDARY_LONG.c_str(), no_argument, nullptr, SECONDARY_VAL},
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
    case SECONDARY_VAL:
      ctx.secondary = true;
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

  if (ctx.secondary && (argc - optind) != 6) {
    throw PtException{"must specify BOTTOM, MIDDLE, TOP, BOTTOM_PRIMARY, MIDDLE_PRIMARY, TOP_PRIMARY layer args, see "
                      "`porytiles compile --help'"};
  }
  else if (!ctx.secondary && (argc - optind) != 3) {
    throw PtException{"must specify BOTTOM, MIDDLE, TOP layer args, see `porytiles compile --help'"};
  }

  if (ctx.secondary) {
    ctx.inputPaths.bottomSecondaryTilesheetPath = argv[optind++];
    ctx.inputPaths.middleSecondaryTilesheetPath = argv[optind++];
    ctx.inputPaths.topSecondaryTilesheetPath = argv[optind++];
  }
  ctx.inputPaths.bottomPrimaryTilesheetPath = argv[optind++];
  ctx.inputPaths.middlePrimaryTilesheetPath = argv[optind++];
  ctx.inputPaths.topPrimaryTilesheetPath = argv[optind++];

  ctx.validate();
}

} // namespace porytiles