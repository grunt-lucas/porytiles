#include "tscreate.h"

#include <iostream>
#include <getopt.h>
#include <png.hpp>
#include <filesystem>

namespace tscreate {
const char* const PROGRAM_NAME = "tscreate";
const char* const VERSION = "0.0.1";
const char* const RELEASE_DATE = "1 June 2023";

bool gVerboseOutput = false;
std::string_view gStructureFilePath;
std::string_view gTransparentColorOpt;
std::string_view gMasterFilePath;
std::string_view gOutputPath;

std::string errorPrefix() {
    std::string program(PROGRAM_NAME);
    return program + ": error: ";
}

void printUsage(std::ostream& outStream) {
    using std::endl;
    outStream << "Usage:  " << PROGRAM_NAME;
    outStream << " [-hstvV] ";
    outStream << "<master.png> <output-dir>" << endl;
}

void printHelp(std::ostream& outStream) {
    using std::endl;
    outStream << PROGRAM_NAME << ", version " << VERSION << " of " << RELEASE_DATE << endl;
    outStream << "   by grunt-lucas: https://github.com/grunt-lucas/tscreate" << endl;
    outStream << endl;
    outStream << "Convert a master PNG tilesheet and optional structure file to a pokeemerald-ready indexed" << endl;
    outStream << "tileset PNG with matching palette files." << endl;
    outStream << endl;
    printUsage(outStream);
    outStream << endl;
    outStream << "Options:" << endl;
    outStream << "   -s, --structure-file=<file>      Specify a structure PNG file." << endl;
    outStream << "   -t, --transparent-color=<R,G,B>  Specify the global transparent color. Defaults to 0,0,0." << endl;
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
    const char* const shortOptions = "hs:t:vV";
    static struct option longOptions[] =
    {
        {"help", no_argument, nullptr, 'h'},
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
        case 's':
            gStructureFilePath = optarg;
            break;
        case 't':
            gTransparentColorOpt = optarg;
            break;
        case 'v':
            printVersion();
            exit(0);
            break;
        case 'V':
            gVerboseOutput = true;
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

    gMasterFilePath = argv[optind++];
    gOutputPath = argv[optind];
}
}

int main(int argc, char** argv) {
    // Parse CLI options and args, fills out global opt vars with expected values
    tscreate::parseOptions(argc, argv);

    try {
        std::string masterFilePath(tscreate::gMasterFilePath);
        std::string outputPath(tscreate::gOutputPath);
        png::image<png::rgb_pixel> image(masterFilePath);
        std::filesystem::create_directory(outputPath);
        std::filesystem::path parentDirectory(outputPath);
        std::filesystem::path outputFile("output.png");
        image.write(parentDirectory / outputFile);
    }
    catch (std::exception& e) {
        std::cerr << tscreate::errorPrefix() << e.what() << std::endl;
        exit(1);
    }

    return 0;
}
