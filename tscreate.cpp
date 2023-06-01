#include "tscreate.h"

#include <iostream>
#include <getopt.h>

namespace tscreate {
const char* const PROGRAM_NAME = "tscreate";
const char* const VERSION = "0.0.1";
const char* const RELEASE_DATE = "1 June 2023";

bool gVerboseOutput = false;

void printHelp() {
    using std::cout;
    using std::endl;
    cout << PROGRAM_NAME << ", version " << VERSION << " of " << RELEASE_DATE << endl;
    cout << "   by grunt-lucas: https://github.com/grunt-lucas/tscreate" << endl;
    cout << endl;
    cout << "Convert a master PNG tilesheet and optional structure file to a pokeemerald-ready indexed" << endl;
    cout << "tileset PNG with matching palette files." << endl;
    cout << endl;
    cout << "Usage:  " << PROGRAM_NAME;
    cout << " [-hstvV] ";
    cout << "<master.png> <output-dir>" << endl;
    cout << endl;
    cout << "Options:" << endl;
    cout << "   -s, --structure-file=<file>      Specify a structure PNG file." << endl;
    cout << "   -t, --transparent-color=<R,G,B>  Specify the global transparent color. Defaults to 0,0,0." << endl;
    cout << endl;
    cout << "Help and Logging:" << endl;
    cout << "   -h, --help     Print help message." << endl;
    cout << "   -v, --version  Print version info." << endl;
    cout << "   -V, --verbose  Enable verbose logging to stderr." << endl;
}

void printVersion() {
    using std::cout;
    using std::endl;
    cout << PROGRAM_NAME << " " << VERSION << endl;
}
}

int main(int argc, char** argv) {
    const char* const shortOptions = "hvV";
    static struct option longOptions[] =
    {
        {"help", no_argument, nullptr, 'h'},
        {"version", no_argument, nullptr, 'v'},
        {"verbose", no_argument, nullptr, 'V'},
        {nullptr, no_argument, nullptr, 0}
    };

    while (true) {
        const auto opt = getopt_long(argc, argv, shortOptions, longOptions, nullptr);

        if (opt == -1)
            break;

        switch (opt) {
        case 'v':
            tscreate::printVersion();
            exit(0);
            break;
        case 'V':
            tscreate::gVerboseOutput = true;
            break;

        // Help message upon '-h/--help'
        case 'h':
            tscreate::printHelp();
            exit(0);
        // Help message on invalid or unknown options
        case '?':
        default:
            tscreate::printHelp();
            exit(2);
        }
    }

    return 0;
}
