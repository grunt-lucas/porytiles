#include <exception>
#include <porytiles/build_version.h>
#include <porytiles/diagnostics/diagnostic_engine.hpp>
#include <porytiles/diagnostics/diagnostics.hpp>
#include <porytiles/legacy/cli_parser.h>
#include <porytiles/legacy/driver.h>
#include <porytiles/legacy/logger.h>
#include <porytiles/legacy/porytiles_context.h>
#include <porytiles/legacy/porytiles_exception.h>

int main(int argc, char **argv) try {
    porytiles1::PorytilesContext ctx{};
    auto engine = std::make_unique<porytiles1::DiagEngine>(std::make_unique<porytiles1::StderrConsumer>());
    ctx.set_diag_engine(std::move(engine));
    parseOptions(ctx, argc, argv);
    drive(ctx);

    const auto warn_count = ctx.diag->InFlightCountForLevel(porytiles1::DiagLevel::Warning);
    if (warn_count == 1) {
        porytiles1::pt_println(stderr, "{} warning generated.", warn_count);
    } else if (warn_count > 1) {
        porytiles1::pt_println(stderr, "{} warnings generated.", warn_count);
    }

    return 0;
} catch (const porytiles1::PorytilesException &e) {
    /*
     * Catch PorytilesException here. This exception is used by the error system to indicate an error it correctly
     * handled and reported to the user. These errors are typically due to invalid user input. So we can just return
     1
     * here to indicate a bad exit.
     */
    return 1;
} catch (const std::exception &e) {
    /*
     * Any other exception type indicates an internal compiler error, i.e. an error we did not explicitly handle or
     * anticipate from library code, or an error we explicitly threw due to an unrecoverable assert failure. This
     * usually indicates a bug in the compiler. Just dump a helpful message so the user can file an issue on GitHub.
     */

    /*
     * FEATURE : New C++23 features may allow a stacktrace here: https://github.com/TylerGlaiel/Crashlogs
     * Or do something like this: https://stackoverflow.com/questions/691719/c-display-stack-trace-on-exception
     */
    porytiles1::pt_println(
        stderr, "{}: {} {}", PORYTILES_EXECUTABLE,
        fmt::styled("internal compiler error:", fmt::emphasis::bold | fg(fmt::terminal_color::yellow)), e.what());
    porytiles1::pt_println(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    porytiles1::pt_println(stderr, "This is a bug. Please file an issue here:");
    porytiles1::pt_println(stderr, "https://github.com/grunt-lucas/porytiles/issues");
    porytiles1::pt_println(stderr, "");
    porytiles1::pt_println(stderr, "In the issue body, please include the following info:");
    porytiles1::pt_println(stderr, "  - the above error message");
    porytiles1::pt_println(stderr, "  - the full command line you ran");
    porytiles1::pt_println(stderr, "  - any relevant input files");
    porytiles1::pt_println(stderr, "  - the version / commit of Porytiles you are using");
    porytiles1::pt_println(stderr, "  - the compiler (and settings) you built with (if you built from source)");
    porytiles1::pt_println(stderr, "");
    porytiles1::pt_println(stderr, "Including these items makes it more likely a maintainer will be able to");
    porytiles1::pt_println(stderr, "reproduce the issue and create a fix release.");
    return 1;
}