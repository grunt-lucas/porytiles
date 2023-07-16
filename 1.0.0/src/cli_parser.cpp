#include "cli_parser.h"

#include <iostream>
#include <sstream>
#include <iterator>
#include <getopt.h>

namespace porytiles {

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

/*
 * Subcommands and options
 */

/*
 * Definitions for `compile-raw'
 */
const std::string COMPILE_RAW_COMMAND = "compile-raw";

/*
 * Definitions for `compile'
 */
const std::string COMPILE_COMMAND = "compile";
//constexpr int TRIPLE_LAYER = 1000;


void parseOptions(Config& config, int argc, char** argv) {
    std::ostringstream implodedGlobalShorts;
    std::copy(GLOBAL_SHORTS.begin(), GLOBAL_SHORTS.end(),
           std::ostream_iterator<std::string>(implodedGlobalShorts, ""));
    const std::string globalShortOptions = implodedGlobalShorts.str();
    static struct option globalLongOptions[] =
            {
                    {HELP_LONG.c_str(),              no_argument,       nullptr, HELP_SHORT[0]},
                    {VERBOSE_LONG.c_str(),           no_argument,       nullptr, VERBOSE_SHORT[0]},
                    {VERSION_LONG.c_str(),           no_argument,       nullptr, VERSION_SHORT[0]},
                    {nullptr,                        no_argument,       nullptr, 0}
            };

    // leading '+' tells getopt to follow posix and stop the loop at first non-option arg
    std::string globalOptions = "+" + globalShortOptions;
    while (true) {
        const auto opt = getopt_long_only(argc, argv, globalOptions.c_str(), globalLongOptions, nullptr);

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
                std::cout << "porytiles help message" << std::endl;
                exit(0);
                // Help message on invalid or unknown options goes to stderr and gives error code
            case '?':
            default:
                std::cout << "porytiles help message" << std::endl;
                exit(2);
        }
    }

    std::cout << "argc " << argc << std::endl;
    std::cout << "optind " << optind << std::endl;

    if ((argc - optind) == 0) {
        std::cout << "subcommand required" << std::endl;
        exit(1);
    }

    std::string subcommand = argv[optind++];
    std::cout << "subcommand was: " << subcommand << std::endl;
}

}