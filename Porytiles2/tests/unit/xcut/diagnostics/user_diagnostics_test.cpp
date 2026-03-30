#include "gtest/gtest.h"

#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/result/error.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles2;

// MockError is a non-FormattableError type used for testing
class MockError : public Error {
  public:
    explicit MockError(std::string msg) : msg_{std::move(msg)} {}

    [[nodiscard]] std::vector<std::string> details(const TextFormatter &formatter) const override
    {
        return {msg_};
    }

    [[nodiscard]] std::unique_ptr<Error> clone() const override
    {
        return std::make_unique<MockError>(msg_);
    }

  private:
    std::string msg_;
};

TEST(UserDiagnosticsTests, FatalFiltersEmptyFormattableErrors)
{
    ChainableResult<int> root_result{FormattableError{"root cause error"}};

    // Chain with an empty error (passthrough)
    ChainableResult<std::string> middle_result{FormattableError{}, root_result};

    // Chain with another real error
    ChainableResult<void> top_result{FormattableError{"proximate error"}, middle_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 0);

    ASSERT_FALSE(diagnostics.fatal_proximates()[0].empty());
    EXPECT_NE(diagnostics.fatal_proximates()[0][0].find("proximate error"), std::string::npos);
    ASSERT_FALSE(diagnostics.fatal_roots()[0].empty());
    EXPECT_NE(diagnostics.fatal_roots()[0][0].find("root cause error"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalPreservesNonEmptyFormattable)
{
    ChainableResult<int> root_result{FormattableError{"root cause"}};
    ChainableResult<std::string> middle_result{FormattableError{"middle layer"}, root_result};
    ChainableResult<void> top_result{FormattableError{"top layer"}, middle_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);

    ASSERT_FALSE(diagnostics.fatal_proximates()[0].empty());
    EXPECT_NE(diagnostics.fatal_proximates()[0][0].find("top layer"), std::string::npos);
    ASSERT_FALSE(diagnostics.fatal_steps()[0].empty());
    EXPECT_NE(diagnostics.fatal_steps()[0][0].find("middle layer"), std::string::npos);
    ASSERT_FALSE(diagnostics.fatal_roots()[0].empty());
    EXPECT_NE(diagnostics.fatal_roots()[0][0].find("root cause"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalMultipleEmptyErrorsInChain)
{
    ChainableResult<int> root_result{FormattableError{"actual root"}};
    ChainableResult<std::string> empty1_result{FormattableError{}, root_result};
    ChainableResult<double> empty2_result{FormattableError{}, empty1_result};
    ChainableResult<void> top_result{FormattableError{"actual proximate"}, empty2_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 0);

    ASSERT_FALSE(diagnostics.fatal_proximates()[0].empty());
    EXPECT_NE(diagnostics.fatal_proximates()[0][0].find("actual proximate"), std::string::npos);
    ASSERT_FALSE(diagnostics.fatal_roots()[0].empty());
    EXPECT_NE(diagnostics.fatal_roots()[0][0].find("actual root"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalSingleNonEmptyError)
{
    ChainableResult<void> result{FormattableError{"single error"}};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(result);

    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 0);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 0);

    ASSERT_FALSE(diagnostics.fatal_proximates()[0].empty());
    EXPECT_NE(diagnostics.fatal_proximates()[0][0].find("single error"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalIncludesNonFormattable)
{
    ChainableResult<void, MockError> result{MockError{"custom error message"}};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(result);

    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 0);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 0);

    ASSERT_FALSE(diagnostics.fatal_proximates()[0].empty());
    EXPECT_NE(diagnostics.fatal_proximates()[0][0].find("custom error message"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalMixedFormattableAndNonFormattable)
{
    ChainableResult<int, MockError> root_result{MockError{"mock root error"}};
    ChainableResult<std::string, FormattableError> middle_result{
        FormattableError{"formattable middle error"}, root_result};
    ChainableResult<void, MockError> top_result{MockError{"mock proximate error"}, middle_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);

    ASSERT_FALSE(diagnostics.fatal_proximates()[0].empty());
    EXPECT_NE(diagnostics.fatal_proximates()[0][0].find("mock proximate error"), std::string::npos);
    ASSERT_FALSE(diagnostics.fatal_steps()[0].empty());
    EXPECT_NE(diagnostics.fatal_steps()[0][0].find("formattable middle error"), std::string::npos);
    ASSERT_FALSE(diagnostics.fatal_roots()[0].empty());
    EXPECT_NE(diagnostics.fatal_roots()[0][0].find("mock root error"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalFiltersEmptyKeepsNonFormattable)
{
    ChainableResult<int, MockError> root_result{MockError{"mock root"}};
    ChainableResult<std::string, FormattableError> empty1_result{FormattableError{}, root_result};
    ChainableResult<double, MockError> middle_result{MockError{"mock middle"}, empty1_result};
    ChainableResult<float, FormattableError> empty2_result{FormattableError{}, middle_result};
    ChainableResult<void, MockError> top_result{MockError{"mock proximate"}, empty2_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);

    ASSERT_FALSE(diagnostics.fatal_proximates()[0].empty());
    EXPECT_NE(diagnostics.fatal_proximates()[0][0].find("mock proximate"), std::string::npos);
    ASSERT_FALSE(diagnostics.fatal_steps()[0].empty());
    EXPECT_NE(diagnostics.fatal_steps()[0][0].find("mock middle"), std::string::npos);
    ASSERT_FALSE(diagnostics.fatal_roots()[0].empty());
    EXPECT_NE(diagnostics.fatal_roots()[0][0].find("mock root"), std::string::npos);
}

TEST(UserDiagnosticsTests, FatalChainOnlyNonFormattable)
{
    ChainableResult<int, MockError> root_result{MockError{"non-formattable root"}};
    ChainableResult<std::string, MockError> middle_result{MockError{"non-formattable middle"}, root_result};
    ChainableResult<void, MockError> top_result{MockError{"non-formattable proximate"}, middle_result};

    BufferedUserDiagnostics diagnostics{};
    diagnostics.fatal(top_result);

    EXPECT_EQ(diagnostics.fatal_proximates().size(), 1);
    EXPECT_EQ(diagnostics.fatal_steps().size(), 1);
    EXPECT_EQ(diagnostics.fatal_roots().size(), 1);

    ASSERT_FALSE(diagnostics.fatal_proximates()[0].empty());
    EXPECT_NE(diagnostics.fatal_proximates()[0][0].find("non-formattable proximate"), std::string::npos);
    ASSERT_FALSE(diagnostics.fatal_steps()[0].empty());
    EXPECT_NE(diagnostics.fatal_steps()[0][0].find("non-formattable middle"), std::string::npos);
    ASSERT_FALSE(diagnostics.fatal_roots()[0].empty());
    EXPECT_NE(diagnostics.fatal_roots()[0][0].find("non-formattable root"), std::string::npos);
}
