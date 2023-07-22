#include "cli_parser.h"

#include <iostream>
#include <sstream>
#include <string>
#include <iterator>
#include <filesystem>
#include <getopt.h>

#include "ptexception.h"
#include "config.h"

namespace porytiles {

const std::string PROGRAM_NAME = "porytiles";
const std::string VERSION = "1.0.0-SNAPSHOT";
const std::string RELEASE_DATE = "---";

static void parseGlobalOptions(Config& config, int argc, char** argv);
static void parseSubcommand(Config& config, int argc, char** argv);
static void parseCompileRaw(Config& config, int argc, char** argv);
static void parseCompile(Config& config, int argc, char** argv);

void parseOptions(Config& config, int argc, char** argv) {
    parseGlobalOptions(config, argc, argv);
    parseSubcommand(config, argc, argv);

    switch(config.subcommand) {
    case COMPILE_RAW:
        parseCompileRaw(config, argc, argv);
        break;
    case COMPILE:
        parseCompile(config, argc, argv);
        break;
    }
}

template<typename T>
static T parseIntegralOption(const std::string& optionName, const char* optarg) {
    try {
        size_t pos;
        T arg = std::stoi(optarg, &pos);
        if (std::string{optarg}.size() != pos) {
            throw PtException{"option argument was not a valid integral type"};
        }
        return arg;
    }
    catch (const std::exception& e) {
        throw PtException{
                "invalid argument `" + std::string{optarg} + "' for option `" + optionName + "': " + e.what()};
    }
}

static TilesPngPaletteMode parseTilesPngPaletteMode(const std::string& optionName, const char* optarg) {
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

/*
 * TODO : everything here is very preliminary and should be hardened for a better user experience
 */


// ----------------------------
// |    OPTION DEFINITIONS    |
// ----------------------------
/*
 * We'll define all the options and help strings here to reduce code repetition. Some options will be shared between
 * subcommands so we want to avoid duplicating message strings, etc.
 */
const std::string HELP_LONG = "help";
constexpr char HELP_SHORT = "h"[0];
const std::string HELP_DESCRIPTION =
"    -" + std::string{HELP_SHORT} + ", --" + HELP_LONG + "\n"
"        Print help message.\n";

const std::string VERBOSE_LONG = "verbose";
constexpr char VERBOSE_SHORT = "v"[0];
const std::string VERBOSE_DESCRIPTION =
"    -" + std::string{VERBOSE_SHORT} + ", --" + VERBOSE_LONG + "\n"
"        Enable verbose logging to stdout.\n";

const std::string VERSION_LONG = "version";
constexpr char VERSION_SHORT = "V"[0];
const std::string VERSION_DESCRIPTION =
"    -" + std::string{VERSION_SHORT} + ", --" + VERSION_LONG + "\n"
"        Print version info.\n";

const std::string OUTPUT_LONG = "output";
constexpr char OUTPUT_SHORT = "o"[0];
const std::string OUTPUT_DESCRIPTION =
"    -" + std::string{OUTPUT_SHORT} + ", --" + OUTPUT_LONG + "=<PATH>\n"
"        Output build files to the directory specified by PATH. If any element\n"
"        of PATH does not exist, it will be created. Defaults to the current\n"
"        working directory (i.e. `.').\n";

const std::string NUM_TILES_IN_PRIMARY_LONG = "num-tiles-primary";
const std::string NUM_TILES_IN_PRIMARY_DESCRIPTION =
"    --" + NUM_TILES_IN_PRIMARY_LONG + "=<N>\n"
"        Set the number of tiles in a primary set to N. This value should match\n"
"        the corresponding value in your project's `include/fieldmap.h'.\n"
"        Defaults to 512 (the pokeemerald default).\n";
const int NUM_TILES_IN_PRIMARY_VAL = 1000;

const std::string NUM_TILES_TOTAL_LONG = "num-tiles-total";
const std::string NUM_TILES_TOTAL_DESCRIPTION =
"    --" + NUM_TILES_TOTAL_LONG + "=<N>\n"
"        Set the total number of tiles (primary + secondary) to N. This value\n"
"        should match the corresponding value in your project's\n"
"        `include/fieldmap.h'. Defaults to 1024 (the pokeemerald default).\n";
const int NUM_TILES_TOTAL_VAL = 1001;

const std::string NUM_PALETTES_IN_PRIMARY_LONG = "num-pals-primary";
const std::string NUM_PALETTES_IN_PRIMARY_DESCRIPTION =
"    --" + NUM_PALETTES_IN_PRIMARY_LONG + "=<N>\n"
"        Set the number of palettes in a primary set to N. This value should\n"
"        match the corresponding value in your project's `include/fieldmap.h'.\n"
"        Defaults to 6 (the pokeemerald default).\n";
const int NUM_PALETTES_IN_PRIMARY_VAL = 1002;

const std::string NUM_PALETTES_TOTAL_LONG = "num-pals-total";
const std::string NUM_PALETTES_TOTAL_DESCRIPTION =
"    --" + NUM_PALETTES_TOTAL_LONG + "=<N>\n"
"        Set the total number of palettes (primary + secondary) to N. This\n"
"        value should match the corresponding value in your project's\n"
"        `include/fieldmap.h'. Defaults to 13 (the pokeemerald default).\n";
const int NUM_PALETTES_TOTAL_VAL = 1003;

const std::string TILES_PNG_PALETTE_MODE_LONG = "tiles-png-pal-mode";
const std::string TILES_PNG_PALETTE_MODE_DESCRIPTION =
"    --" + TILES_PNG_PALETTE_MODE_LONG + "=<MODE>\n"
"        Set the palette mode for the output `tiles.png'. Valid settings are\n"
"        `pal0', `true-color', or `greyscale'. These settings are for human\n"
"        visual purposes only and have no effect on the final in-game tiles.\n"
"        Default value is `greyscale'.\n";
const int TILES_PNG_PALETTE_MODE_VAL = 1004;

const std::string SECONDARY_LONG = "secondary";
const std::string SECONDARY_DESCRIPTION =
"    --" + SECONDARY_LONG + "\n"
"        Specify that this tileset should be treated as a secondary tileset.\n"
"        Secondary tilesets are able to reuse tiles and palettes from their\n"
"        paired primary tileset. Note: the paired primary tileset must be\n"
"        a Porytiles-handled tileset.\n";
const int SECONDARY_VAL = 1005;


// --------------------------------
// |    GLOBAL OPTION PARSING     |
// --------------------------------

const std::vector<std::string> GLOBAL_SHORTS = {std::string{HELP_SHORT}, std::string{VERBOSE_SHORT}, std::string{VERSION_SHORT}};
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
HELP_DESCRIPTION +
"\n" +
VERBOSE_DESCRIPTION +
"\n" +
VERSION_DESCRIPTION +
"\n"
"Commands:\n"
"    compile-raw\n"
"        Compile a raw tilesheet. Won't generate a `metatiles.bin'.\n"
"\n"
"    compile\n"
"        Compile three layer RGBA PNGs into a complete tileset.\n"
"\n"
"Run `porytiles COMMAND --help' for more information about a command.\n"
"\n"
"To get more help with porytiles, check out the guides at:\n"
"    https://github.com/grunt-lucas/porytiles/wiki/Porytiles-Homepage\n";

static void parseGlobalOptions(Config& config, int argc, char** argv) {
    std::ostringstream implodedShorts;
    std::copy(GLOBAL_SHORTS.begin(), GLOBAL_SHORTS.end(),
           std::ostream_iterator<std::string>(implodedShorts, ""));
    // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
    std::string shortOptions = "+" + implodedShorts.str();
    static struct option longOptions[] =
            {
                    {HELP_LONG.c_str(),              no_argument,       nullptr, HELP_SHORT},
                    {VERBOSE_LONG.c_str(),           no_argument,       nullptr, VERBOSE_SHORT},
                    {VERSION_LONG.c_str(),           no_argument,       nullptr, VERSION_SHORT},
                    {nullptr,                        no_argument,       nullptr, 0}
            };

    while (true) {
        const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

        if (opt == -1)
            break;

        switch (opt) {
            case VERBOSE_SHORT:
                std::cout << "verbose mode activated" << std::endl;
                break;
            case VERSION_SHORT:
                std::cout << PROGRAM_NAME << " " << VERSION << " " << RELEASE_DATE << std::endl;
                exit(0);

                // Help message upon '-h/--help' goes to stdout
            case HELP_SHORT:
                std::cout << GLOBAL_HELP << std::endl;
                exit(0);
                // Help message on invalid or unknown options goes to stderr and gives error code
            case '?':
            default:
                std::cout << GLOBAL_HELP << std::endl;
                exit(2);
        }
    }
}


// ----------------------------
// |    SUBCOMMAND PARSING    |
// ----------------------------

const std::string COMPILE_RAW_COMMAND = "compile-raw";
const std::string COMPILE_COMMAND = "compile";
static void parseSubcommand(Config& config, int argc, char** argv) {
    if ((argc - optind) == 0) {
        throw PtException{"missing required subcommand, try `porytiles --help' for usage information"};
    }

    std::string subcommand = argv[optind++];
    if (subcommand == COMPILE_RAW_COMMAND) {
        config.subcommand = Subcommand::COMPILE_RAW;
    }
    else if (subcommand == COMPILE_COMMAND) {
        config.subcommand = Subcommand::COMPILE;
    }
    else {
        throw PtException{"unrecognized subcommand: " + subcommand};
    }
}


// -----------------------------
// |    COMPILE-RAW COMMAND    |
// -----------------------------

const std::vector<std::string> COMPILE_RAW_SHORTS = {std::string{HELP_SHORT}, std::string{OUTPUT_SHORT} + ":"};
const std::string COMPILE_RAW_HELP =
"Usage:\n"
"    porytiles " + COMPILE_RAW_COMMAND + " [OPTIONS] TILES\n"
"\n"
"Compile a raw tilesheet, i.e. a single tilesheet with no layer information.\n"
"This command will not generate a `metatiles.bin' file.\n"
"\n"
"Args:\n"
"    <TILES>\n"
"        An RGBA PNG tilesheet containing pixel art to be tile-ized.\n"
"\n"
"Options:\n" +
OUTPUT_DESCRIPTION +
"\n" +
NUM_TILES_IN_PRIMARY_DESCRIPTION +
"\n" +
NUM_TILES_TOTAL_DESCRIPTION +
"\n" +
NUM_PALETTES_IN_PRIMARY_DESCRIPTION +
"\n" +
NUM_PALETTES_TOTAL_DESCRIPTION +
"\n" +
TILES_PNG_PALETTE_MODE_DESCRIPTION +
"\n";

static void parseCompileRaw(Config& config, int argc, char** argv) {
    std::ostringstream implodedShorts;
    std::copy(COMPILE_RAW_SHORTS.begin(), COMPILE_RAW_SHORTS.end(),
           std::ostream_iterator<std::string>(implodedShorts, ""));
    // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
    std::string shortOptions = "+" + implodedShorts.str();
    static struct option longOptions[] =
            {
                    {HELP_LONG.c_str(),                    no_argument,       nullptr, HELP_SHORT},
                    {NUM_PALETTES_IN_PRIMARY_LONG.c_str(), required_argument, nullptr, NUM_PALETTES_IN_PRIMARY_VAL},
                    {NUM_PALETTES_TOTAL_LONG.c_str(),      required_argument, nullptr, NUM_PALETTES_TOTAL_VAL},
                    {NUM_TILES_IN_PRIMARY_LONG.c_str(),    required_argument, nullptr, NUM_TILES_IN_PRIMARY_VAL},
                    {NUM_TILES_TOTAL_LONG.c_str(),         required_argument, nullptr, NUM_TILES_TOTAL_VAL},
                    {OUTPUT_LONG.c_str(),                  required_argument, nullptr, OUTPUT_SHORT},
                    {TILES_PNG_PALETTE_MODE_LONG.c_str(),  required_argument, nullptr, TILES_PNG_PALETTE_MODE_VAL},
                    {nullptr,                              no_argument,       nullptr, 0}
            };

    while (true) {
        const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

        if (opt == -1)
            break;

        switch (opt) {
            case OUTPUT_SHORT:
                config.outputPath = optarg;
                break;
            case NUM_PALETTES_IN_PRIMARY_VAL:
                config.numPalettesInPrimary = parseIntegralOption<size_t>(NUM_PALETTES_IN_PRIMARY_LONG, optarg);
                break;
            case NUM_PALETTES_TOTAL_VAL:
                config.numPalettesTotal = parseIntegralOption<size_t>(NUM_PALETTES_TOTAL_LONG, optarg);
                break;
            case NUM_TILES_IN_PRIMARY_VAL:
                config.numTilesInPrimary = parseIntegralOption<size_t>(NUM_TILES_IN_PRIMARY_LONG, optarg);
                break;
            case NUM_TILES_TOTAL_VAL:
                config.numTilesTotal = parseIntegralOption<size_t>(NUM_TILES_TOTAL_LONG, optarg);
                break;
            case TILES_PNG_PALETTE_MODE_VAL:
                config.tilesPngPaletteMode = parseTilesPngPaletteMode(TILES_PNG_PALETTE_MODE_LONG, optarg);
                break;

            // Help message upon '-h/--help' goes to stdout
            case HELP_SHORT:
                std::cout << COMPILE_RAW_HELP << std::endl;
                exit(0);
            // Help message on invalid or unknown options goes to stderr and gives error code
            case '?':
            default:
                std::cout << COMPILE_RAW_HELP << std::endl;
                exit(2);
        }
    }

    if ((argc - optind) != 1) {
        throw PtException{"must specify exactly 1 TILES arg, see `porytiles compile-raw --help'"};
    }

    config.rawTilesheetPath = argv[optind++];
}


// -------------------------
// |    COMPILE COMMAND    |
// -------------------------

const std::vector<std::string> COMPILE_SHORTS = {std::string{HELP_SHORT}, std::string{OUTPUT_SHORT} + ":"};
const std::string COMPILE_HELP =
"Usage:\n"
"    porytiles " + COMPILE_COMMAND + " [OPTIONS] BOTTOM MIDDLE TOP\n"
"    porytiles " + COMPILE_COMMAND + " [OPTIONS] --secondary BOTTOM MIDDLE TOP BOTTOM-PRIMARY MIDDLE-PRIMARY TOP-PRIMARY\n"
"\n"
"Compile a bottom, middle, and top tilesheet into a complete tileset. This\n"
"command will generate a `metatiles.bin' file along with `tiles.png' and the\n"
"pal files.\n"
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
OUTPUT_DESCRIPTION +
"\n" +
NUM_TILES_IN_PRIMARY_DESCRIPTION +
"\n" +
NUM_TILES_TOTAL_DESCRIPTION +
"\n" +
NUM_PALETTES_IN_PRIMARY_DESCRIPTION +
"\n" +
NUM_PALETTES_TOTAL_DESCRIPTION +
"\n" +
TILES_PNG_PALETTE_MODE_DESCRIPTION +
"\n" +
SECONDARY_DESCRIPTION +
"\n";

static void parseCompile(Config& config, int argc, char** argv) {
    std::ostringstream implodedShorts;
    std::copy(COMPILE_SHORTS.begin(), COMPILE_SHORTS.end(),
           std::ostream_iterator<std::string>(implodedShorts, ""));
    // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
    std::string shortOptions = "+" + implodedShorts.str();
    static struct option longOptions[] =
            {
                    {HELP_LONG.c_str(),                    no_argument,       nullptr, HELP_SHORT},
                    {NUM_PALETTES_IN_PRIMARY_LONG.c_str(), required_argument, nullptr, NUM_PALETTES_IN_PRIMARY_VAL},
                    {NUM_PALETTES_TOTAL_LONG.c_str(),      required_argument, nullptr, NUM_PALETTES_TOTAL_VAL},
                    {NUM_TILES_IN_PRIMARY_LONG.c_str(),    required_argument, nullptr, NUM_TILES_IN_PRIMARY_VAL},
                    {NUM_TILES_TOTAL_LONG.c_str(),         required_argument, nullptr, NUM_TILES_TOTAL_VAL},
                    {OUTPUT_LONG.c_str(),                  required_argument, nullptr, OUTPUT_SHORT},
                    {TILES_PNG_PALETTE_MODE_LONG.c_str(),  required_argument, nullptr, TILES_PNG_PALETTE_MODE_VAL},
                    {SECONDARY_LONG.c_str(),               no_argument,       nullptr, SECONDARY_VAL},
                    {nullptr,                              no_argument,       nullptr, 0}
            };

    while (true) {
        const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

        if (opt == -1)
            break;

        switch (opt) {
            case OUTPUT_SHORT:
                config.outputPath = optarg;
                break;
            case NUM_PALETTES_IN_PRIMARY_VAL:
                config.numPalettesInPrimary = parseIntegralOption<size_t>(NUM_PALETTES_IN_PRIMARY_LONG, optarg);
                break;
            case NUM_PALETTES_TOTAL_VAL:
                config.numPalettesTotal = parseIntegralOption<size_t>(NUM_PALETTES_TOTAL_LONG, optarg);
                break;
            case NUM_TILES_IN_PRIMARY_VAL:
                config.numTilesInPrimary = parseIntegralOption<size_t>(NUM_TILES_IN_PRIMARY_LONG, optarg);
                break;
            case NUM_TILES_TOTAL_VAL:
                config.numTilesTotal = parseIntegralOption<size_t>(NUM_TILES_TOTAL_LONG, optarg);
                break;
            case TILES_PNG_PALETTE_MODE_VAL:
                config.tilesPngPaletteMode = parseTilesPngPaletteMode(TILES_PNG_PALETTE_MODE_LONG, optarg);
                break;
            case SECONDARY_VAL:
                config.secondary = true;
                break;

            // Help message upon '-h/--help' goes to stdout
            case HELP_SHORT:
                std::cout << COMPILE_HELP << std::endl;
                exit(0);
            // Help message on invalid or unknown options goes to stderr and gives error code
            case '?':
            default:
                std::cout << COMPILE_HELP << std::endl;
                exit(2);
        }
    }

    if (config.secondary && (argc - optind) != 6) {
        throw PtException{"secondary mode must specify exactly 6 layer args, see `porytiles compile --help'"};
    }
    else if ((argc - optind) != 3) {
        throw PtException{"primary mdoe must specify exactly 3 layer args, see `porytiles compile --help'"};
    }

    config.bottomTilesheetPath = argv[optind++];
    config.middleTilesheetPath = argv[optind++];
    config.topTilesheetPath = argv[optind++];

    if (config.secondary) {
        config.bottomPrimaryTilesheetPath = argv[optind++];
        config.middlePrimaryTilesheetPath = argv[optind++];
        config.topPrimaryTilesheetPath = argv[optind++];
    }
}

}