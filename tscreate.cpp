#include "tscreate.h"

#include <iostream>
#include <getopt.h>

namespace tscreate {
const char* const PROGRAM_NAME = "tscreate";
const char* const VERSION = "0.0.1";
const char* const RELEASE_DATE = "1 June 2023";

bool gVerboseOutput = false;
std::string_view gStructureFilePath;

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

        // Help message upon '-h/--help'
        case 'h':
            printHelp(std::cout);
            exit(0);
        // Help message on invalid or unknown options
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
}
}

int main(int argc, char** argv) {
    tscreate::parseOptions(argc, argv);

    return 0;
}
