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
    auto details = error.details(formatter);
    EXPECT_TRUE(details.empty());
}

TEST(FormattableErrorTests, EmptyStringConstructorShouldReturnEmptyDetails)
{
    FormattableError error{""};
    AnsiStyledTextFormatter formatter{};
    auto details = error.details(formatter);
    EXPECT_TRUE(details.empty());
}

TEST(FormattableErrorTests, NonEmptyStringConstructorShouldReturnMessage)
{
    FormattableError error{"something went wrong"};
    AnsiStyledTextFormatter formatter{};
    auto details = error.details(formatter);
    ASSERT_EQ(details.size(), 1);
    EXPECT_EQ(details[0], "something went wrong");
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

TEST(FormattableErrorTests, MultiLineConstructorShouldReturnAllLines)
{
    std::vector<std::string> lines{"line 1", "line 2", "line 3"};
    FormattableError error{lines};
    AnsiStyledTextFormatter formatter{};
    auto details = error.details(formatter);
    ASSERT_EQ(details.size(), 3);
    EXPECT_EQ(details[0], "line 1");
    EXPECT_EQ(details[1], "line 2");
    EXPECT_EQ(details[2], "line 3");
}

TEST(FormattableErrorTests, MultiLineConstructorShouldHaveDetails)
{
    std::vector<std::string> lines{"line 1", "line 2"};
    FormattableError error{lines};
    EXPECT_TRUE(error.has_details());
}

TEST(FormattableErrorTests, MultiLineWithEmptyLinesShouldHaveDetailsIfAnyNonEmpty)
{
    std::vector<std::string> lines{"", "non-empty", ""};
    FormattableError error{lines};
    EXPECT_TRUE(error.has_details());
}

TEST(FormattableErrorTests, MultiLineWithAllEmptyLinesShouldNotHaveDetails)
{
    std::vector<std::string> lines{"", "", ""};
    FormattableError error{lines};
    EXPECT_FALSE(error.has_details());
}

TEST(FormattableErrorTests, MultiLineWithParamsShouldFormatEachLine)
{
    std::vector<std::string> lines{"error: {}", "value: {}"};
    std::vector<std::vector<FormatParam>> params{
        {FormatParam{"first", Style::bold}}, {FormatParam{"second", Style::bold}}};
    FormattableError error{lines, params};
    AnsiStyledTextFormatter formatter{};
    auto details = error.details(formatter);
    ASSERT_EQ(details.size(), 2);
    // The formatted strings will contain ANSI codes, so just check they're not empty and contain the text
    EXPECT_NE(details[0].find("error:"), std::string::npos);
    EXPECT_NE(details[0].find("first"), std::string::npos);
    EXPECT_NE(details[1].find("value:"), std::string::npos);
    EXPECT_NE(details[1].find("second"), std::string::npos);
}

TEST(FormattableErrorTests, MultiLineWithFewerParamsThanLinesShouldWorkCorrectly)
{
    std::vector<std::string> lines{"error: {}", "plain line", "another: {}"};
    std::vector<std::vector<FormatParam>> params{{FormatParam{"first", Style::bold}}};
    FormattableError error{lines, params};
    AnsiStyledTextFormatter formatter{};
    auto details = error.details(formatter);
    ASSERT_EQ(details.size(), 3);
    EXPECT_NE(details[0].find("first"), std::string::npos);
    EXPECT_EQ(details[1], "plain line");
    EXPECT_EQ(details[2], "another: {}");
}

TEST(FormattableErrorTests, CloneShouldPreserveMultiLineErrors)
{
    std::vector<std::string> lines{"line 1", "line 2"};
    FormattableError error{lines};
    auto cloned = error.clone();
    const auto *cloned_formattable = dynamic_cast<const FormattableError *>(cloned.get());
    ASSERT_NE(cloned_formattable, nullptr);
    EXPECT_TRUE(cloned_formattable->has_details());

    AnsiStyledTextFormatter formatter{};
    auto details = cloned_formattable->details(formatter);
    ASSERT_EQ(details.size(), 2);
    EXPECT_EQ(details[0], "line 1");
    EXPECT_EQ(details[1], "line 2");
}
