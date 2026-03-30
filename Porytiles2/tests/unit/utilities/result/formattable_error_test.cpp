#include "gtest/gtest.h"

#include "porytiles2/utilities/result/error.hpp"
#include "porytiles2/utilities/text/ansi_styled_text_formatter.hpp"

using namespace porytiles2;

TEST(FormattableErrorTests, DefaultCtorNoDetails)
{
    FormattableError error{};
    EXPECT_FALSE(error.has_details());
}

TEST(FormattableErrorTests, EmptyStringCtorNoDetails)
{
    FormattableError error{""};
    EXPECT_FALSE(error.has_details());
}

TEST(FormattableErrorTests, NonEmptyStringCtorHasDetails)
{
    FormattableError error{"something went wrong"};
    EXPECT_TRUE(error.has_details());
}

TEST(FormattableErrorTests, FormattedMsgCtorHasDetails)
{
    FormattableError error{"error: {}", FormatParam{"test", Style::bold}};
    EXPECT_TRUE(error.has_details());
}

TEST(FormattableErrorTests, DefaultMessage)
{
    FormattableError error{};
    AnsiStyledTextFormatter formatter{};
    auto details = error.details(formatter);
    EXPECT_TRUE(details.empty());
}

TEST(FormattableErrorTests, EmptyStringMessage)
{
    FormattableError error{""};
    AnsiStyledTextFormatter formatter{};
    auto details = error.details(formatter);
    EXPECT_TRUE(details.empty());
}

TEST(FormattableErrorTests, NonEmptyStringMessage)
{
    FormattableError error{"something went wrong"};
    AnsiStyledTextFormatter formatter{};
    auto details = error.details(formatter);
    ASSERT_EQ(details.size(), 1);
    EXPECT_EQ(details[0], "something went wrong");
}

TEST(FormattableErrorTests, ClonePreservesHasDetails)
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

TEST(FormattableErrorTests, MultiLineMessage)
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

TEST(FormattableErrorTests, MultiLineCtorHasDetails)
{
    std::vector<std::string> lines{"line 1", "line 2"};
    FormattableError error{lines};
    EXPECT_TRUE(error.has_details());
}

TEST(FormattableErrorTests, MultiLineMixedEmptyHasDetails)
{
    std::vector<std::string> lines{"", "non-empty", ""};
    FormattableError error{lines};
    EXPECT_TRUE(error.has_details());
}

TEST(FormattableErrorTests, MultiLineAllEmptyNoDetails)
{
    std::vector<std::string> lines{"", "", ""};
    FormattableError error{lines};
    EXPECT_FALSE(error.has_details());
}

TEST(FormattableErrorTests, MultiLineFormatParams)
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

TEST(FormattableErrorTests, MultiLineFewerParams)
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

TEST(FormattableErrorTests, ClonePreservesMultiLine)
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

// Style Tests - Foreground and Background Color Combinations

TEST(StyleTests, ForegroundOnly)
{
    const Style fg_red = Style::red;
    EXPECT_TRUE(fg_red.has_fg_color());
    EXPECT_FALSE(fg_red.has_bg_color());
    EXPECT_FALSE(fg_red.is_fg_rgb());
    EXPECT_EQ(fg_red.fg_predefined(), PredefinedColor::red);
}

TEST(StyleTests, BackgroundOnly)
{
    const Style bg_blue = Style::bg_blue;
    EXPECT_FALSE(bg_blue.has_fg_color());
    EXPECT_TRUE(bg_blue.has_bg_color());
    EXPECT_FALSE(bg_blue.is_bg_rgb());
    EXPECT_EQ(bg_blue.bg_predefined(), PredefinedColor::blue);
}

TEST(StyleTests, ForegroundAndBackground)
{
    const Style combined = Style::red | Style::bg_blue;
    EXPECT_TRUE(combined.has_fg_color());
    EXPECT_TRUE(combined.has_bg_color());
    EXPECT_FALSE(combined.is_fg_rgb());
    EXPECT_FALSE(combined.is_bg_rgb());
    EXPECT_EQ(combined.fg_predefined(), PredefinedColor::red);
    EXPECT_EQ(combined.bg_predefined(), PredefinedColor::blue);
}

TEST(StyleTests, BoldWithForegroundAndBackground)
{
    const Style styled = Style::bold | Style::red | Style::bg_yellow;
    EXPECT_TRUE(styled.has_bold());
    EXPECT_TRUE(styled.has_fg_color());
    EXPECT_TRUE(styled.has_bg_color());
    EXPECT_FALSE(styled.is_fg_rgb());
    EXPECT_FALSE(styled.is_bg_rgb());
    EXPECT_EQ(styled.fg_predefined(), PredefinedColor::red);
    EXPECT_EQ(styled.bg_predefined(), PredefinedColor::yellow);
}

TEST(StyleTests, RgbForegroundPredefinedBackground)
{
    const Style combined = rgb_fg_style(255, 128, 0) | Style::bg_black;
    EXPECT_TRUE(combined.has_fg_color());
    EXPECT_TRUE(combined.has_bg_color());
    EXPECT_TRUE(combined.is_fg_rgb());
    EXPECT_FALSE(combined.is_bg_rgb());
    const RgbColor fg = combined.fg_rgb();
    EXPECT_EQ(fg.r, 255);
    EXPECT_EQ(fg.g, 128);
    EXPECT_EQ(fg.b, 0);
    EXPECT_EQ(combined.bg_predefined(), PredefinedColor::black);
}

TEST(StyleTests, PredefinedForegroundRgbBackground)
{
    const Style combined = Style::green | rgb_bg_style(64, 64, 64);
    EXPECT_TRUE(combined.has_fg_color());
    EXPECT_TRUE(combined.has_bg_color());
    EXPECT_FALSE(combined.is_fg_rgb());
    EXPECT_TRUE(combined.is_bg_rgb());
    EXPECT_EQ(combined.fg_predefined(), PredefinedColor::green);
    const RgbColor bg = combined.bg_rgb();
    EXPECT_EQ(bg.r, 64);
    EXPECT_EQ(bg.g, 64);
    EXPECT_EQ(bg.b, 64);
}

TEST(StyleTests, RgbForegroundAndRgbBackground)
{
    const Style combined = rgb_fg_style(255, 0, 0) | rgb_bg_style(0, 0, 255);
    EXPECT_TRUE(combined.has_fg_color());
    EXPECT_TRUE(combined.has_bg_color());
    EXPECT_TRUE(combined.is_fg_rgb());
    EXPECT_TRUE(combined.is_bg_rgb());
    const RgbColor fg = combined.fg_rgb();
    EXPECT_EQ(fg.r, 255);
    EXPECT_EQ(fg.g, 0);
    EXPECT_EQ(fg.b, 0);
    const RgbColor bg = combined.bg_rgb();
    EXPECT_EQ(bg.r, 0);
    EXPECT_EQ(bg.g, 0);
    EXPECT_EQ(bg.b, 255);
}

TEST(StyleTests, RgbStyleAliasCreatesRgbForeground)
{
    const Style style1 = rgb_style(100, 150, 200);
    const Style style2 = rgb_fg_style(100, 150, 200);
    EXPECT_TRUE(style1.is_fg_rgb());
    EXPECT_FALSE(style1.is_bg_rgb());
    EXPECT_TRUE(style2.is_fg_rgb());
    EXPECT_FALSE(style2.is_bg_rgb());
    const RgbColor rgb1 = style1.fg_rgb();
    const RgbColor rgb2 = style2.fg_rgb();
    EXPECT_EQ(rgb1.r, rgb2.r);
    EXPECT_EQ(rgb1.g, rgb2.g);
    EXPECT_EQ(rgb1.b, rgb2.b);
}

// AnsiStyledTextFormatter Tests - Foreground and Background

TEST(AnsiStyledTextFormatterTests, ForegroundOnly)
{
    AnsiStyledTextFormatter formatter{AnsiColorMode::plain};
    const std::string result = formatter.style("test", Style::red);
    EXPECT_NE(result.find("\033[31m"), std::string::npos); // Foreground red ANSI code
    EXPECT_NE(result.find("test"), std::string::npos);
    EXPECT_NE(result.find("\033[0m"), std::string::npos); // Reset code
}

TEST(AnsiStyledTextFormatterTests, BackgroundOnly)
{
    AnsiStyledTextFormatter formatter{AnsiColorMode::plain};
    const std::string result = formatter.style("test", Style::bg_blue);
    EXPECT_NE(result.find("\033[44m"), std::string::npos); // Background blue ANSI code
    EXPECT_NE(result.find("test"), std::string::npos);
    EXPECT_NE(result.find("\033[0m"), std::string::npos); // Reset code
}

TEST(AnsiStyledTextFormatterTests, ForegroundAndBackground)
{
    AnsiStyledTextFormatter formatter{AnsiColorMode::plain};
    const std::string result = formatter.style("test", Style::red | Style::bg_blue);
    EXPECT_NE(result.find("\033[31m"), std::string::npos); // Foreground red ANSI code
    EXPECT_NE(result.find("\033[44m"), std::string::npos); // Background blue ANSI code
    EXPECT_NE(result.find("test"), std::string::npos);
    EXPECT_NE(result.find("\033[0m"), std::string::npos); // Reset code
}

TEST(AnsiStyledTextFormatterTests, Rgb24BitForegroundAndBackground)
{
    AnsiStyledTextFormatter formatter{AnsiColorMode::colors_24_bit};
    const Style combined = rgb_fg_style(255, 128, 0) | rgb_bg_style(64, 64, 64);
    const std::string result = formatter.style("test", combined);
    EXPECT_NE(result.find("\033[38;2;255;128;0m"), std::string::npos); // 24-bit foreground
    EXPECT_NE(result.find("\033[48;2;64;64;64m"), std::string::npos);  // 24-bit background
    EXPECT_NE(result.find("test"), std::string::npos);
    EXPECT_NE(result.find("\033[0m"), std::string::npos); // Reset code
}

TEST(AnsiStyledTextFormatterTests, BoldWithForegroundAndBackground)
{
    AnsiStyledTextFormatter formatter{AnsiColorMode::plain};
    const std::string result = formatter.style("test", Style::bold | Style::green | Style::bg_yellow);
    EXPECT_NE(result.find("\033[32m"), std::string::npos); // Foreground green ANSI code
    EXPECT_NE(result.find("\033[43m"), std::string::npos); // Background yellow ANSI code
    EXPECT_NE(result.find("\033[1m"), std::string::npos);  // Bold ANSI code
    EXPECT_NE(result.find("test"), std::string::npos);
    EXPECT_NE(result.find("\033[0m"), std::string::npos); // Reset code
}
