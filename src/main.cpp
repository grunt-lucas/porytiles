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

int main(int argc, char** argv) try {
    porytiles::Config config = porytiles::defaultConfig();
    porytiles::parseOptions(config, argc, argv);
    porytiles::drive(config);
    return 0;
}
catch (const porytiles::PtException& e) {
    // Catch PtException here, these are errors that can reasonably be expected due to bad input, bad files, etc
    fmt::println(stderr, "{}: {} {}", porytiles::PROGRAM_NAME,
                 fmt::styled("error:", fmt::emphasis::bold | fg(fmt::terminal_color::red)), e.what());
    return 1;
}
catch (const std::exception& e) {
    // TODO : if this catches, something we really didn't expect happened, can we print a stack trace here? How?
    // New C++23 features may allow this at some point: https://github.com/TylerGlaiel/Crashlogs
    // Or do something like this: https://stackoverflow.com/questions/691719/c-display-stack-trace-on-exception
    fmt::println(stderr, "{}: {} {}", porytiles::PROGRAM_NAME,
                 fmt::styled("fatal:", fmt::emphasis::bold | fg(fmt::terminal_color::red)), e.what());
    fmt::println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    fmt::println("This is a bug. Please file an issue here: https://github.com/grunt-lucas/porytiles/issues");
    fmt::println("Be sure to include the full command you ran, as well as any accompanying input files that");
    fmt::println("trigger the error. This way a maintainer can reproduce the issue.");
    return 1;
}
