#include "gtest/gtest.h"

#include "porytiles2/utilities/text/ansi_styled_text_formatter.hpp"
#include "porytiles2/xcut/result/error.hpp"

using namespace porytiles2;

TEST(FormattableErrorTests, DefaultConstructorShouldHaveNoDetails)
{
    FormattableError error{};
    EXPECT_FALSE(error.has_details());
}

TEST(FormattableErrorTests, EmptyStringConstructorShouldHaveNoDetails)
{
    FormattableError error{""};
    EXPECT_FALSE(error.has_details());
}

TEST(FormattableErrorTests, NonEmptyStringConstructorShouldHaveDetails)
{
    FormattableError error{"something went wrong"};
    EXPECT_TRUE(error.has_details());
}

TEST(FormattableErrorTests, FormattedMessageConstructorShouldHaveDetails)
{
    FormattableError error{"error: {}", FormatParam{"test", Style::bold}};
    EXPECT_TRUE(error.has_details());
}

TEST(FormattableErrorTests, DefaultConstructorShouldReturnEmptyDetails)
{
    FormattableError error{};
    AnsiStyledTextFormatter formatter{};
    EXPECT_EQ(error.details(formatter), "");
}

TEST(FormattableErrorTests, EmptyStringConstructorShouldReturnEmptyDetails)
{
    FormattableError error{""};
    AnsiStyledTextFormatter formatter{};
    EXPECT_EQ(error.details(formatter), "");
}

TEST(FormattableErrorTests, NonEmptyStringConstructorShouldReturnMessage)
{
    FormattableError error{"something went wrong"};
    AnsiStyledTextFormatter formatter{};
    EXPECT_EQ(error.details(formatter), "something went wrong");
}

TEST(FormattableErrorTests, CloneShouldPreserveHasDetailsState)
{
    FormattableError empty_error{};
    auto cloned_empty = empty_error.clone();
    const auto *cloned_formattable = dynamic_cast<const FormattableError *>(cloned_empty.get());
    ASSERT_NE(cloned_formattable, nullptr);
    EXPECT_FALSE(cloned_formattable->has_details());

    FormattableError non_empty_error{"test error"};
    auto cloned_non_empty = non_empty_error.clone();
    const auto *cloned_non_empty_formattable = dynamic_cast<const FormattableError *>(cloned_non_empty.get());
    ASSERT_NE(cloned_non_empty_formattable, nullptr);
    EXPECT_TRUE(cloned_non_empty_formattable->has_details());
}
