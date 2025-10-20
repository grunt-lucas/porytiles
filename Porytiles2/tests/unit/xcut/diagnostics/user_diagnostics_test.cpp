#include "gtest/gtest.h"

#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"
#include "porytiles2/xcut/result/error.hpp"

using namespace porytiles2;

// MockError is a non-FormattableError type used for testing
class MockError : public Error {
  public:
    explicit MockError(std::string msg) : msg_{std::move(msg)} {}

    [[nodiscard]] std::string details(const TextFormatter &formatter) const override
    {
        return msg_;
    }

    [[nodiscard]] std::unique_ptr<Error> clone() const override
    {
        return std::make_unique<MockError>(msg_);
    }

  private:
    std::string msg_;
};

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

TEST(UserDiagnosticsTests, FatalShouldIncludeNonFormattableErrors)
{
    // Create a chain with a single MockError (non-FormattableError)
    ChainableResult<void, MockError> result{MockError{"custom error message"}};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(result);

    // Should have the MockError as proximate
    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 0);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 0);

    // Verify the MockError appears in output
    EXPECT_NE(diagnostics.fatal_proximates()[0].find("custom error message"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalShouldHandleMixedFormattableAndNonFormattableErrors)
{
    // Create a chain with both FormattableError and MockError (non-FormattableError)
    ChainableResult<int, MockError> root_result{MockError{"mock root error"}};
    ChainableResult<std::string, FormattableError> middle_result{
        FormattableError{"formattable middle error"}, root_result};
    ChainableResult<void, MockError> top_result{MockError{"mock proximate error"}, middle_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    // Should have all 3 errors preserved
    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);

    // Verify all errors appear in output
    EXPECT_NE(diagnostics.fatal_proximates()[0].find("mock proximate error"), std::string::npos);
    EXPECT_NE(diagnostics.fatal_steps()[0].find("formattable middle error"), std::string::npos);
    EXPECT_NE(diagnostics.fatal_roots()[0].find("mock root error"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalShouldFilterEmptyFormattableErrorsButKeepNonFormattableErrors)
{
    // Create a chain with empty FormattableErrors and non-FormattableErrors interspersed
    ChainableResult<int, MockError> root_result{MockError{"mock root"}};
    ChainableResult<std::string, FormattableError> empty1_result{FormattableError{}, root_result};
    ChainableResult<double, MockError> middle_result{MockError{"mock middle"}, empty1_result};
    ChainableResult<float, FormattableError> empty2_result{FormattableError{}, middle_result};
    ChainableResult<void, MockError> top_result{MockError{"mock proximate"}, empty2_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    // Should filter out empty FormattableErrors but keep all MockErrors
    // Expecting 3 errors: proximate, step, root
    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);

    // Verify all MockErrors appear in output
    EXPECT_NE(diagnostics.fatal_proximates()[0].find("mock proximate"), std::string::npos);
    EXPECT_NE(diagnostics.fatal_steps()[0].find("mock middle"), std::string::npos);
    EXPECT_NE(diagnostics.fatal_roots()[0].find("mock root"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalShouldHandleChainWithOnlyNonFormattableErrors)
{
    // Create a chain with only MockErrors (no FormattableErrors)
    ChainableResult<int, MockError> root_result{MockError{"non-formattable root"}};
    ChainableResult<std::string, MockError> middle_result{MockError{"non-formattable middle"}, root_result};
    ChainableResult<void, MockError> top_result{MockError{"non-formattable proximate"}, middle_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    // Should preserve all errors with proper categorization
    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);

    // Verify proper categorization
    EXPECT_NE(diagnostics.fatal_proximates()[0].find("non-formattable proximate"), std::string::npos);
    EXPECT_NE(diagnostics.fatal_steps()[0].find("non-formattable middle"), std::string::npos);
    EXPECT_NE(diagnostics.fatal_roots()[0].find("non-formattable root"), std::string::npos);
}
