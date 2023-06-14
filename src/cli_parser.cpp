#include "cli_parser.h"

#include "tileset.h"
#include "tsexception.h"
#include "rgb_color.h"

#include <string>
#include <iostream>
#include <getopt.h>
#include <filesystem>

namespace porytiles {
const char* const PROGRAM_NAME = "porytiles";
const char* const VERSION = "0.0.1";
const char* const RELEASE_DATE = "12 June 2023";

// Defaults for unsupplied options
static bool VERBOSE_DEFAULT = false;
static const RgbColor TRANSPARENCY_DEFAULT = {255, 0, 255};
static const RgbColor PRIMER_DEFAULT = {0, 0, 0};
static const RgbColor SIBLING_DEFAULT = {255, 255, 255};
static int MAX_PALETTE_DEFAULT = 6;
static int MAX_TILES_DEFAULT = 512;

// Options
bool gOptVerboseOutput = VERBOSE_DEFAULT;
RgbColor gOptTransparentColor = TRANSPARENCY_DEFAULT;
RgbColor gOptPrimerColor = PRIMER_DEFAULT;
RgbColor gOptSiblingColor = SIBLING_DEFAULT;
size_t gOptMaxPalettes = MAX_PALETTE_DEFAULT;
size_t gOptMaxTiles = MAX_TILES_DEFAULT;

// Arguments (required)
std::string gArgMasterPngPath;
std::string gArgOutputPath;

static void printUsage(std::ostream& outStream) {
    using std::endl;
    outStream << "Usage:  " << PROGRAM_NAME;
    outStream << " [-hnpstTvV] ";
    outStream << "<master.png> <output-dir>" << endl;
}

// @formatter:off
static void printHelp(std::ostream& outStream) {
    using std::endl;
    outStream << PROGRAM_NAME << ", version " << VERSION << " of " << RELEASE_DATE << endl;
    outStream << "   by grunt-lucas: https://github.com/grunt-lucas/porytiles" << endl;
    outStream << endl;
    outStream << "Convert an RGB master PNG tilesheet to a pokeemerald-ready 4bpp indexed tileset PNG" << endl;
    outStream << "with matching palette files. See the repo wiki for more detailed usage information."
              << endl;
    outStream << endl;
    printUsage(outStream);
    outStream << endl;
    outStream << "Options:" << endl;
    outStream
            << "   -n, --max-palettes=<num>         Specify the maximum number of palettes porytiles is allowed" << endl
            << "                                    to allocate (default: " << MAX_PALETTE_DEFAULT << ")." << endl;
    outStream << endl;
    outStream
            << "   -T, --max-tiles=<num>            Specify the maximum number of tiles porytiles is allowed" << endl
            << "                                    to generate (default: " << MAX_TILES_DEFAULT << ")." << endl;
    outStream << endl;
    outStream
            << "   -p, --primer-color=<R,G,B>       Specify the tile color for the primer control tile" << endl
            << "                                    (default: " << PRIMER_DEFAULT.prettyString() << "). See wiki for more info." << endl;
    outStream << endl;
    outStream
            << "   -s, --sibling-color=<R,G,B>      Specify the tile color for the sibling control tile" << endl
            << "                                    (default: " << SIBLING_DEFAULT.prettyString() << "). See wiki for more info." << endl;
    outStream << endl;
    outStream
            << "   -t, --transparent-color=<R,G,B>  Specify the global transparent color (default: " << TRANSPARENCY_DEFAULT.prettyString() << ")." << endl;
    outStream << endl;
    outStream << "Help and Logging:" << endl;
    outStream << "   -h, --help     Print help message." << endl;
    outStream << "   -v, --verbose  Enable verbose logging to stdout." << endl;
    outStream << "   -V, --version  Print version info." << endl;
}
// @formatter:on

static void printVersion() {
    using std::cout;
    using std::endl;
    cout << PROGRAM_NAME << " " << VERSION << endl;
}

static size_t parseSizeOption(const std::string& optionName, const char* optarg) {
    try {
        size_t pos;
        size_t arg = std::stoi(optarg, &pos);
        if (std::string{optarg}.size() != pos) {
            throw TsException{"option argument was not a valid integral type"};
        }
        return arg;
    }
    catch (const std::exception& e) {
        throw TsException{
                "invalid argument `" + std::string{optarg} + "' for option `" + optionName + "': " + e.what()};
    }
}

void parseOptions(int argc, char** argv) {
    const char* const shortOptions = "hn:p:s:t:T:vV";
    static struct option longOptions[] =
            {
                    {"help",              no_argument,       nullptr, 'h'},
                    {"max-palettes",      required_argument, nullptr, 'n'},
                    {"primer-color",      required_argument, nullptr, 'p'},
                    {"sibling-color",     required_argument, nullptr, 's'},
                    {"transparent-color", required_argument, nullptr, 't'},
                    {"max-tiles",         required_argument, nullptr, 'T'},
                    {"verbose",           no_argument,       nullptr, 'v'},
                    {"version",           no_argument,       nullptr, 'V'},
                    {nullptr,             no_argument,       nullptr, 0}
            };

    while (true) {
        const auto opt = getopt_long(argc, argv, shortOptions, longOptions, nullptr);

        if (opt == -1)
            break;

        switch (opt) {
            case 'n':
                gOptMaxPalettes = parseSizeOption("-n, --max-palettes", optarg);
                if (gOptMaxPalettes > NUM_BG_PALS) {
                    throw TsException{"requested " + std::to_string(gOptMaxPalettes) +
                                      " palettes, max allowed: " + std::to_string(NUM_BG_PALS)};
                }
                break;
            case 'p':
                // TODO parse in separate function that can throw appropriate error if invalid arg
                throw TsException{"TODO: --primer-color option unimplemented"};
                // gOptPrimerColor = optarg;
                // break;
            case 's':
                // TODO parse in separate function that can throw appropriate error if invalid arg
                throw TsException{"TODO: --sibling-color option unimplemented"};
                // gOptSiblingColor = optarg;
                // break;
            case 't':
                // TODO parse in separate function that can throw appropriate error if invalid arg
                throw TsException{"TODO: --transparent-color option unimplemented"};
                // gOptTransparentColor = optarg;
                // break;
            case 'T':
                gOptMaxTiles = parseSizeOption("-T, --max-tiles", optarg);
                break;
            case 'v':
                gOptVerboseOutput = true;
                break;
            case 'V':
                printVersion();
                exit(0);

                // Help message upon '-h/--help' goes to stdout
            case 'h':
                printHelp(std::cout);
                exit(0);
                // Help message on invalid or unknown options goes to stderr and gives error code
            case '?':
            default:
                printHelp(std::cerr);
                exit(2);
        }
    }

    const int numRequiredArgs = 2;
    if ((argc - optind) != numRequiredArgs) {
        printUsage(std::cerr);
        exit(1);
    }

    gArgMasterPngPath = argv[optind++];
    gArgOutputPath = argv[optind];

    if (!std::filesystem::exists(gArgMasterPngPath)) {
        throw TsException{gArgMasterPngPath + ": file does not exist"};
    }

    if (!std::filesystem::is_regular_file(gArgMasterPngPath)) {
        throw TsException{gArgMasterPngPath + ": file exists but is a directory"};
    }

    if (std::filesystem::exists(gArgOutputPath) && !std::filesystem::is_directory(gArgOutputPath)) {
        throw TsException{gArgOutputPath + ": exists but is not a directory"};
    }
}
} // namespace porytiles
