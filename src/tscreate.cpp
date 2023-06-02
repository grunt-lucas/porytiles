#include "tscreate.h"

#include "init_checks.h"

#include <iostream>
#include <getopt.h>
#include <png.hpp>
#include <filesystem>

namespace tscreate {
static const char* const PROGRAM_NAME = "tscreate";
static const char* const VERSION = "0.0.1";
static const char* const RELEASE_DATE = "1 June 2023";

// Defaults for unsupplied options
static const char* const MAX_PALETTE_DEFAULT = "6";
static const char* const TRANSPARENCY_DEFAULT = "0,0,0";

bool gOptVerboseOutput = false;
std::string_view gOptStructureFilePath;
std::string_view gOptTransparentColor;
std::string_view gOptMaxPalettes;

std::string_view gArgMasterPngPath;
std::string_view gArgOutputPath;

std::string errorPrefix() {
    std::string program(PROGRAM_NAME);
    return program + ": error: ";
}

void printUsage(std::ostream& outStream) {
    using std::endl;
    outStream << "Usage:  " << PROGRAM_NAME;
    outStream << " [-hpstvV] ";
    outStream << "<master.png> <output-dir>" << endl;
}

void printHelp(std::ostream& outStream) {
    using std::endl;
    outStream << PROGRAM_NAME << ", version " << VERSION << " of " << RELEASE_DATE << endl;
    outStream << "   by grunt-lucas: https://github.com/grunt-lucas/tscreate" << endl;
    outStream << endl;
    outStream << "Convert a master PNG tilesheet and optional structure file to a pokeemerald-ready indexed" << endl;
    outStream << "tileset PNG with matching palette files. See the repo wiki for more detailed usage information." << endl;
    outStream << endl;
    printUsage(outStream);
    outStream << endl;
    outStream << "Options:" << endl;
    outStream << "   -p, --max-palettes=<num>         Specify the maximum number of palettes tscreate is allowed to allocate (default: "
        << MAX_PALETTE_DEFAULT << ")." << endl;
    outStream << "   -s, --structure-file=<file>      Specify a structure PNG file. See wiki for more info." << endl;
    outStream << "   -t, --transparent-color=<R,G,B>  Specify the global transparent color (default: "
        << TRANSPARENCY_DEFAULT << ")." << endl;
    outStream << endl;
    outStream << "Help and Logging:" << endl;
    outStream << "   -h, --help     Print help message." << endl;
    outStream << "   -v, --version  Print version info." << endl;
    outStream << "   -V, --verbose  Enable verbose logging to stderr." << endl;
}

void printVersion() {
    using std::cout;
    using std::endl;
    cout << PROGRAM_NAME << " " << VERSION << endl;
}

void parseOptions(int argc, char** argv) {
    const char* const shortOptions = "hp:s:t:vV";
    static struct option longOptions[] =
    {
        {"help", no_argument, nullptr, 'h'},
        {"max-palettes", required_argument, nullptr, 'p'},
        {"structure-file", required_argument, nullptr, 's'},
        {"transparent-color", required_argument, nullptr, 't'},
        {"version", no_argument, nullptr, 'v'},
        {"verbose", no_argument, nullptr, 'V'},
        {nullptr, no_argument, nullptr, 0}
    };

    while (true) {
        const auto opt = getopt_long(argc, argv, shortOptions, longOptions, nullptr);

        if (opt == -1)
            break;

        switch (opt) {
        case 'p':
            gOptMaxPalettes = optarg;
            break;
        case 's':
            gOptStructureFilePath = optarg;
            break;
        case 't':
            gOptTransparentColor = optarg;
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

    // Set default values for certain options if they weren't supplied
    if (gOptMaxPalettes.empty()) {
        gOptMaxPalettes = MAX_PALETTE_DEFAULT;
    }
    if (gOptTransparentColor.empty()) {
        gOptTransparentColor = TRANSPARENCY_DEFAULT;
    }

    const int numRequiredArgs = 2;
    if ((argc - optind) != numRequiredArgs) {
        printUsage(std::cerr);
        exit(1);
    }

    gArgMasterPngPath = argv[optind++];
    gArgOutputPath = argv[optind];
}
}

int main(int argc, char** argv) try {
    // Parse CLI options and args, fills out global opt vars with expected values
    tscreate::parseOptions(argc, argv);
    std::string masterPngPath(tscreate::gArgMasterPngPath);
    std::string outputPath(tscreate::gArgOutputPath);

    // Checks master PNG exists and validates its dimensions (must be divisible by 8 to hold tiles)
    tscreate::validateMasterPngExistsAndDimensions(masterPngPath);

    // Create output directory if possible, otherwise fail
    std::filesystem::create_directory(outputPath);

    // TODO : remove this test code that simply writes the master PNG to output dir
    std::filesystem::path parentDirectory(outputPath);
    std::filesystem::path outputFile("output.png");
    png::image<png::rgb_pixel> masterPng(masterPngPath);
    masterPng.write(parentDirectory / outputFile);

    return 0;
}
catch (std::exception& e) {
    std::cerr << tscreate::errorPrefix() << e.what() << std::endl;
    return 1;
}
