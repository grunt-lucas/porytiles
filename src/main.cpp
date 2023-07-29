#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

#include <exception>

#define FMT_HEADER_ONLY
#include "fmt/color.h"

#include "config.h"
#include "driver.h"
#include "cli_parser.h"
#include "ptexception.h"
#include "program_name.h"
#include "logger.h"

int main(int argc, char** argv) try {
    porytiles::Config config = porytiles::defaultConfig();
    porytiles::parseOptions(config, argc, argv);
    porytiles::drive(config);
    return 0;
}
catch (const porytiles::PtException& e) {
    // Catch PtException here, these are errors that can reasonably be expected due to bad input, bad files, etc
    fmt::println(stderr, "{}", e.what());
    return 1;
}
catch (const std::exception& e) {
    // TODO : if this catches, something we really didn't expect happened, can we print a stack trace here? How?
    // New C++23 features may allow this at some point: https://github.com/TylerGlaiel/Crashlogs
    // Or do something like this: https://stackoverflow.com/questions/691719/c-display-stack-trace-on-exception
    fmt::println(stderr, "{}: {} {}", porytiles::PROGRAM_NAME,
                 fmt::styled("internal compiler error:", fmt::emphasis::bold | fg(fmt::terminal_color::yellow)), e.what());
    fmt::println(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    fmt::println(stderr, "This is a bug. Please file an issue here:");
    fmt::println(stderr, "https://github.com/grunt-lucas/porytiles/issues");
    fmt::println(stderr, "");
    fmt::println(stderr, "In the issue body, please include the following info:");
    fmt::println(stderr, "  - the above error message");
    fmt::println(stderr, "  - the full command line you ran");
    fmt::println(stderr, "  - any relevant input files");
    fmt::println(stderr, "  - the version / commit of Porytiles you are using");
    fmt::println(stderr, "  - the compiler (and settings) you built with (if you built from source)");
    fmt::println(stderr, "");
    fmt::println(stderr, "Including these items makes it more likely a maintainer will be able to");
    fmt::println(stderr, "reproduce the issue and create a patch.");
    return 1;
}
