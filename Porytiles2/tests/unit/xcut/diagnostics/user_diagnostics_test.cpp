#include "gtest/gtest.h"

#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"
#include "porytiles2/xcut/result/error.hpp"

using namespace porytiles2;

TEST(UserDiagnosticsTests, FatalShouldFilterOutEmptyFormattableErrors)
{
    // Create a chain with empty errors interspersed with real errors
    // Start with a real error at the root
    ChainableResult<int> root_result{FormattableError{"root cause error"}};

    // Chain with an empty error (passthrough)
    ChainableResult<std::string> middle_result{FormattableError{}, root_result};

    // Chain with another real error
    ChainableResult<void> top_result{FormattableError{"proximate error"}, middle_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    // Should have filtered out the empty error, leaving only 2 errors
    // proximate: "proximate error"
    // root: "root cause error"
    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 0);

    // Verify the content
    EXPECT_NE(diagnostics.fatal_proximates()[0].find("proximate error"), std::string::npos);
    EXPECT_NE(diagnostics.fatal_roots()[0].find("root cause error"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalShouldPreserveNonEmptyFormattableErrors)
{
    // Create a chain with all non-empty errors
    ChainableResult<int> root_result{FormattableError{"root cause"}};
    ChainableResult<std::string> middle_result{FormattableError{"middle layer"}, root_result};
    ChainableResult<void> top_result{FormattableError{"top layer"}, middle_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    // Should have all 3 errors preserved
    // proximate: "top layer"
    // step: "middle layer"
    // root: "root cause"
    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);

    // Verify the content
    EXPECT_NE(diagnostics.fatal_proximates()[0].find("top layer"), std::string::npos);
    EXPECT_NE(diagnostics.fatal_steps()[0].find("middle layer"), std::string::npos);
    EXPECT_NE(diagnostics.fatal_roots()[0].find("root cause"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalShouldHandleMultipleEmptyErrorsInChain)
{
    // Create a longer chain with multiple empty errors
    ChainableResult<int> root_result{FormattableError{"actual root"}};
    ChainableResult<std::string> empty1_result{FormattableError{}, root_result};
    ChainableResult<double> empty2_result{FormattableError{}, empty1_result};
    ChainableResult<void> top_result{FormattableError{"actual proximate"}, empty2_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    // Should filter out both empty errors, leaving only 2
    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 0);

    // Verify the content
    EXPECT_NE(diagnostics.fatal_proximates()[0].find("actual proximate"), std::string::npos);
    EXPECT_NE(diagnostics.fatal_roots()[0].find("actual root"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalShouldHandleSingleNonEmptyError)
{
    // Create a simple chain with just one error
    ChainableResult<void> result{FormattableError{"single error"}};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(result);

    // Should have only the proximate error, no root
    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 0);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 0);

    // Verify the content
    EXPECT_NE(diagnostics.fatal_proximates()[0].find("single error"), std::string::npos);
}
