#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

#include <exception>

#include "cli_parser.h"
#include "driver.h"
#include "logger.h"
#include "program_name.h"
#include "ptcontext.h"
#include "ptexception.h"

int main(int argc, char **argv)
try {
  porytiles::PtContext ctx{};
  porytiles::parseOptions(ctx, argc, argv);
  porytiles::drive(ctx);

  if (ctx.err.warnCount == 1) {
    porytiles::pt_println(stderr, "{} warning generated.", ctx.err.warnCount);
  }
  else if (ctx.err.warnCount > 1) {
    porytiles::pt_println(stderr, "{} warnings generated.", ctx.err.warnCount);
  }

  return 0;
}
catch (const porytiles::PtException &e) {
  /*
   * Catch PtException here. This exception is used by the error system to indicate an error it correctly handled and
   * reported to the user. These errors are typically due to invalid user input. So we can just return 1 here to
   * indicate a bad exit.
   */
  return 1;
}
catch (const std::exception &e) {
  /*
   * Any other exception type indicates an internal compiler error, i.e. an error we did not explicitly handle or
   * anticipate from library code, or an error we explicitly threw due to an unrecoverable assert failure. This usually
   * indicates a bug in the compiler. Just dump a helpful message so the user can file an issue on GitHub.
   */

  // New C++23 features may allow a stacktrace here: https://github.com/TylerGlaiel/Crashlogs
  // Or do something like this: https://stackoverflow.com/questions/691719/c-display-stack-trace-on-exception
  porytiles::pt_println(stderr, "{}: {} {}", porytiles::PROGRAM_NAME,
                        fmt::styled("internal compiler error:", fmt::emphasis::bold | fg(fmt::terminal_color::yellow)),
                        e.what());
  porytiles::pt_println(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
  porytiles::pt_println(stderr, "This is a bug. Please file an issue here:");
  porytiles::pt_println(stderr, "https://github.com/grunt-lucas/porytiles/issues");
  porytiles::pt_println(stderr, "");
  porytiles::pt_println(stderr, "In the issue body, please include the following info:");
  porytiles::pt_println(stderr, "  - the above error message");
  porytiles::pt_println(stderr, "  - the full command line you ran");
  porytiles::pt_println(stderr, "  - any relevant input files");
  porytiles::pt_println(stderr, "  - the version / commit of Porytiles you are using");
  porytiles::pt_println(stderr, "  - the compiler (and settings) you built with (if you built from source)");
  porytiles::pt_println(stderr, "");
  porytiles::pt_println(stderr, "Including these items makes it more likely a maintainer will be able to");
  porytiles::pt_println(stderr, "reproduce the issue and create a fix release.");
  return 1;
}
