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
static constexpr bool VERBOSE_DEFAULT = false;
static constexpr bool X8BPP_DEFAULT = false;
static const RgbColor TRANSPARENCY_DEFAULT = {255, 0, 255};
static const RgbColor PRIMER_DEFAULT = {0, 0, 0};
static const RgbColor SIBLING_DEFAULT = {255, 255, 255};
static constexpr int MAX_PALETTE_DEFAULT = 6;
static constexpr int MAX_TILES_DEFAULT = 512;

// Options
bool gOptVerboseOutput = VERBOSE_DEFAULT;
bool gOpt8bppOutput = X8BPP_DEFAULT;
RgbColor gOptTransparentColor = TRANSPARENCY_DEFAULT;
RgbColor gOptPrimerColor = PRIMER_DEFAULT;
RgbColor gOptSiblingColor = SIBLING_DEFAULT;
size_t gOptMaxPalettes = MAX_PALETTE_DEFAULT;
size_t gOptMaxTiles = MAX_TILES_DEFAULT;

// Arguments (required)
std::string gArgMasterPngPath;
std::string gArgOutputPath;

template<typename T>
static T parseIntegralOption(const std::string& optionName, const char* optarg) {
    try {
        size_t pos;
        T arg = std::stoi(optarg, &pos);
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

static std::vector<std::string> split(std::string input, const std::string& delimiter) {
    std::vector<std::string> result;
    size_t pos;
    std::string token;
    while ((pos = input.find(delimiter)) != std::string::npos) {
        token = input.substr(0, pos);
        result.push_back(token);
        input.erase(0, pos + delimiter.length());
    }
    result.push_back(input);
    return result;
}

static RgbColor parseRgbColor(const std::string& colorString) {
    std::vector<std::string> colorComponents = split(colorString, ",");
    if (colorComponents.size() != 3) {
        throw TsException{"RGB color string `" + colorString + "' must have three components"};
    }
    int red = parseIntegralOption<int>(colorString, colorComponents[0].c_str());
    int green = parseIntegralOption<int>(colorString, colorComponents[1].c_str());
    int blue = parseIntegralOption<int>(colorString, colorComponents[2].c_str());

    if (red < 0 || red > 255) {
        throw TsException{"invalid range for red component, must be 0 < red <  256"};
    }
    if (green < 0 || green > 255) {
        throw TsException{"invalid range for green component, must be 0 < red <  256"};
    }
    if (blue < 0 || blue > 255) {
        throw TsException{"invalid range for blue component, must be 0 < red <  256"};
    }

    return RgbColor{static_cast<png::byte>(red), static_cast<png::byte>(green), static_cast<png::byte>(blue)};
}

static void printUsage(std::ostream& outStream) {
    using std::endl;
    outStream << "Usage:  " << PROGRAM_NAME;
    outStream << " [-8hnpstTvV] ";
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
            << "   -8, --8bpp-output                Output the final tileset PNG as a specially constructed 8bpp image" << endl
            << "                                    to allow for accurate color representation in most image editors." << endl
            << "                                    Please see wiki for some disclaimers." << endl;
    outStream << endl;
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

void parseOptions(int argc, char** argv) {
    const char* const shortOptions = "8hn:p:s:t:T:vV";
    static struct option longOptions[] =
            {
                    {"8bpp-output",       no_argument,       nullptr, '8'},
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
            case '8':
                gOpt8bppOutput = true;
                break;
            case 'n':
                gOptMaxPalettes = parseIntegralOption<size_t>("-n, --max-palettes", optarg);
                if (gOptMaxPalettes > NUM_BG_PALS) {
                    throw TsException{"requested " + std::to_string(gOptMaxPalettes) +
                                      " palettes, max allowed: " + std::to_string(NUM_BG_PALS)};
                }
                break;
            case 'p':
                gOptPrimerColor = parseRgbColor(optarg);
                break;
            case 's':
                gOptSiblingColor = parseRgbColor(optarg);
                break;
            case 't':
                gOptTransparentColor = parseRgbColor(optarg);
                break;
            case 'T':
                gOptMaxTiles = parseIntegralOption<size_t>("-T, --max-tiles", optarg);
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

    // Throw an error if primer, sibling, and transparent colors overlap
    std::unordered_set<RgbColor> colors;
    colors.insert(gOptPrimerColor);
    colors.insert(gOptSiblingColor);
    colors.insert(gOptTransparentColor);
    if (colors.size() != 3) {
        throw TsException{"primer, sibling, and transparent tile colors must be specified uniquely"};
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
