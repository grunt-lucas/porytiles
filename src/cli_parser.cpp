#include "cli_parser.h"

#include <iostream>
#include <sstream>
#include <string>
#include <iterator>
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

void parseOptions(Config& config, int argc, char** argv) {
    parseGlobalOptions(config, argc, argv);
    parseSubcommand(config, argc, argv);

    switch(config.subcommand) {
    case COMPILE_RAW:
        parseCompileRaw(config, argc, argv);
        break;
    }
}

/*
 * TODO : everything here is very preliminary and should be hardened for a better user experience
 */


// -------------------------
// |    GLOBAL OPTIONS     |
// -------------------------

const std::string HELP_LONG = "help";
const std::string HELP_SHORT = "h";
const std::string VERBOSE_LONG = "verbose";
const std::string VERBOSE_SHORT = "v";
const std::string VERSION_LONG = "version";
const std::string VERSION_SHORT = "V";
const std::vector<std::string> GLOBAL_SHORTS = {HELP_SHORT, VERBOSE_SHORT, VERSION_SHORT};

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
"Options:\n"
"    -h, --help        Print help message.\n"
"\n"
"    -v, --verbose     Enable verbose logging to stdout.\n"
"\n"
"    -V, --version     Print version info.\n"
"\n"
"Commands:\n"
"    compile-raw       Compile a raw tilesheet. Won't generate a `metatiles.bin'.\n"
"\n"
"Run `porytiles COMMAND --help' for more information about a command.\n"
"\n"
"To get more help with porytiles, check out the guides at:\n"
"    https://github.com/grunt-lucas/porytiles/wiki\n";

static void parseGlobalOptions(Config& config, int argc, char** argv) {
    std::ostringstream implodedShorts;
    std::copy(GLOBAL_SHORTS.begin(), GLOBAL_SHORTS.end(),
           std::ostream_iterator<std::string>(implodedShorts, ""));
    // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
    std::string shortOptions = "+" + implodedShorts.str();
    static struct option longOptions[] =
            {
                    {HELP_LONG.c_str(),              no_argument,       nullptr, HELP_SHORT[0]},
                    {VERBOSE_LONG.c_str(),           no_argument,       nullptr, VERBOSE_SHORT[0]},
                    {VERSION_LONG.c_str(),           no_argument,       nullptr, VERSION_SHORT[0]},
                    {nullptr,                        no_argument,       nullptr, 0}
            };

    while (true) {
        const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

        if (opt == -1)
            break;

        switch (opt) {
            case 'v':
                std::cout << "verbose mode activated" << std::endl;
                break;
            case 'V':
                std::cout << PROGRAM_NAME << " " << VERSION << " " << RELEASE_DATE << std::endl;
                exit(0);

                // Help message upon '-h/--help' goes to stdout
            case 'h':
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
static void parseSubcommand(Config& config, int argc, char** argv) {
    if ((argc - optind) == 0) {
        throw PtException{"missing required subcommand"};
    }

    std::string subcommand = argv[optind++];
    if (subcommand == COMPILE_RAW_COMMAND) {
        config.subcommand = Subcommand::COMPILE_RAW;
    }
    else {
        throw PtException{"unrecognized subcommand: " + subcommand};
    }
}


// -----------------------------
// |    COMPILE-RAW COMMAND    |
// -----------------------------

const std::vector<std::string> COMPILE_RAW_SHORTS = {HELP_SHORT};
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
"Options:\n"
"    --foo        Some random option TODO : remove\n"
"\n";

static void parseCompileRaw(Config& config, int argc, char** argv) {
    std::ostringstream implodedShorts;
    std::copy(COMPILE_RAW_SHORTS.begin(), COMPILE_RAW_SHORTS.end(),
           std::ostream_iterator<std::string>(implodedShorts, ""));
    // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
    std::string shortOptions = "+" + implodedShorts.str();
    static struct option longOptions[] =
            {
                    {HELP_LONG.c_str(),              no_argument,       nullptr, HELP_SHORT[0]},
                    {nullptr,                        no_argument,       nullptr, 0}
            };

    while (true) {
        const auto opt = getopt_long_only(argc, argv, shortOptions.c_str(), longOptions, nullptr);

        if (opt == -1)
            break;

        switch (opt) {
            // Help message upon '-h/--help' goes to stdout
            case 'h':
                std::cout << COMPILE_RAW_HELP << std::endl;
                exit(0);
            // Help message on invalid or unknown options goes to stderr and gives error code
            case '?':
            default:
                std::cout << COMPILE_RAW_HELP << std::endl;
                exit(2);
        }
    }
}

}