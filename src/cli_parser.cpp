#include "cli_parser.h"

#include <iostream>
#include <sstream>
#include <iterator>
#include <getopt.h>

#include "ptexception.h"
#include "config.h"

namespace porytiles {

const char* const PROGRAM_NAME = "porytiles";
const char* const VERSION = "1.0.0-SNAPSHOT";
const char* const RELEASE_DATE = "---";

/*
 * TODO : everything here is very preliminary and should be hardened for a better user experience
 */

/*
 * Global Options
 */
const std::string HELP_LONG = "help";
const std::string HELP_SHORT = "h";
const std::string VERBOSE_LONG = "verbose";
const std::string VERBOSE_SHORT = "v";
const std::string VERSION_LONG = "version";
const std::string VERSION_SHORT = "V";
const std::vector<std::string> GLOBAL_SHORTS = {HELP_SHORT, VERBOSE_SHORT, VERSION_SHORT};

const std::string GLOBAL_HELP =
"porytiles\n"
"v 1.0.0";

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
                std::cout << "porytiles 1.0.0" << std::endl;
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


const std::vector<std::string> COMPILE_RAW_SHORTS = {HELP_SHORT};
const std::string COMPILE_RAW_HELP =
"compile-raw\n"
"fill in more stuff";

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

void parseOptions(Config& config, int argc, char** argv) {
    parseGlobalOptions(config, argc, argv);
    parseSubcommand(config, argc, argv);

    switch(config.subcommand) {
    case COMPILE_RAW:
        parseCompileRaw(config, argc, argv);
        break;
    }
}

}