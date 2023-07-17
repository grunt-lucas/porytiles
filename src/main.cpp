#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

#include <iostream>

#include "config.h"
#include "cli_parser.h"
#include "ptexception.h"

int main(int argc, char** argv) try {
    porytiles::Config config = porytiles::defaultConfig();
    porytiles::parseOptions(config, argc, argv);

    return 0;
}
catch (const porytiles::PtException& e) {
    // Catch PtException here, these are errors that can reasonably be expected due to bad input, bad files, etc
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
}
catch (const std::exception& e) {
    // TODO : if this catches, something we really didn't expect happened, can we print a stack trace here? How?
    // New C++23 features may allow this at some point: https://github.com/TylerGlaiel/Crashlogs
    std::cerr << "fatal: " << e.what() << std::endl;
    std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
              << std::endl;
    std::cerr
            << "This is a bug. Please file an issue here: https://github.com/grunt-lucas/porytiles/issues"
            << std::endl;
    std::cerr << "Be sure to include the full command you ran, as well as any accompanying input files that"
              << std::endl;
    std::cerr << "trigger the error. This way a maintainer can reproduce the issue." << std::endl;
    return 1;
}
