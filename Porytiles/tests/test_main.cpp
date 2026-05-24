#include "gtest/gtest.h"

#include "porytiles/utilities/panic/panic.hpp"

int main(int argc, char **argv)
{
    // Disable stacktrace generation by default for faster test execution.
    // Tests that intentionally trigger panics don't need stacktraces.
    // Run with --enable-stacktrace to re-enable for debugging.
    bool enable_stacktrace = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == "--enable-stacktrace") {
            enable_stacktrace = true;
            break;
        }
    }
    porytiles::set_panic_stacktrace_enabled(enable_stacktrace);

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
