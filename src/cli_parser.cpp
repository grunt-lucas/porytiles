#include "cli_parser.h"

#include "tsexception.h"
#include "rgb_color.h"

#include <string>
#include <iostream>
#include <getopt.h>
#include <filesystem>

namespace tscreate {
const char* const PROGRAM_NAME = "tscreate";
const char* const VERSION = "0.0.1";
const char* const RELEASE_DATE = "1 June 2023";

// Defaults for unsupplied options
static bool VERBOSE_DEFAULT = false;
static const RgbColor TRANSPARENCY_DEFAULT = {251, 39, 228};
static int MAX_PALETTE_DEFAULT = 6;

// Options
bool gOptVerboseOutput = VERBOSE_DEFAULT;
std::string gOptStructureFilePath;
RgbColor gOptTransparentColor = TRANSPARENCY_DEFAULT;
int gOptMaxPalettes = MAX_PALETTE_DEFAULT;

// Arguments (required)
std::string gArgMasterPngPath;
std::string gArgOutputPath;

static void printUsage(std::ostream& outStream) {
    using std::endl;
    outStream << "Usage:  " << PROGRAM_NAME;
    outStream << " [-hpstvV] ";
    outStream << "<master.png> <output-dir>" << endl;
}

static void printHelp(std::ostream& outStream) {
    using std::endl;
    outStream << PROGRAM_NAME << ", version " << VERSION << " of " << RELEASE_DATE << endl;
    outStream << "   by grunt-lucas: https://github.com/grunt-lucas/tscreate" << endl;
    outStream << endl;
    outStream << "Convert a master PNG tilesheet and optional structure file to a pokeemerald-ready indexed" << endl;
    outStream << "tileset PNG with matching palette files. See the repo wiki for more detailed usage information."
              << endl;
    outStream << endl;
    printUsage(outStream);
    outStream << endl;
    outStream << "Options:" << endl;
    outStream
            << "   -p, --max-palettes=<num>         Specify the maximum number of palettes tscreate is allowed to allocate (default: "
            << MAX_PALETTE_DEFAULT << ")." << endl;
    outStream
            << "   -s, --structure-file=<file>      Specify a PNG file for use as a structure key. See wiki for more info."
            << endl;
    outStream << "   -t, --transparent-color=<R,G,B>  Specify the global transparent color (default: "
              << TRANSPARENCY_DEFAULT.prettyString() << ")."
              << endl;
    outStream << endl;
    outStream << "Help and Logging:" << endl;
    outStream << "   -h, --help     Print help message." << endl;
    outStream << "   -v, --version  Print version info." << endl;
    outStream << "   -V, --verbose  Enable verbose logging to stderr." << endl;
}

static void printVersion() {
    using std::cout;
    using std::endl;
    cout << PROGRAM_NAME << " " << VERSION << endl;
}

void parseOptions(int argc, char** argv) {
    const char* const shortOptions = "hp:s:t:vV";
    static struct option longOptions[] =
            {
                    {"help",              no_argument,       nullptr, 'h'},
                    {"max-palettes",      required_argument, nullptr, 'p'},
                    {"structure-file",    required_argument, nullptr, 's'},
                    {"transparent-color", required_argument, nullptr, 't'},
                    {"version",           no_argument,       nullptr, 'v'},
                    {"verbose",           no_argument,       nullptr, 'V'},
                    {nullptr,             no_argument,       nullptr, 0}
            };

    while (true) {
        const auto opt = getopt_long(argc, argv, shortOptions, longOptions, nullptr);

        if (opt == -1)
            break;

        switch (opt) {
            case 'p':
                // TODO parse in separate function that can error
                gOptMaxPalettes = std::stoi(optarg);
                break;
            case 's':
                gOptStructureFilePath = optarg;
                break;
            case 't':
                // TODO parse in separate function that can error
                throw TsException{"TODO: --transparent-color option unimplemented"};
                // gOptTransparentColor = optarg;
                break;
            case 'v':
                printVersion();
                exit(0);
            case 'V':
                gOptVerboseOutput = true;
                break;

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
} // namespace tscreate
