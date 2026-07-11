#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "porytiles/utilities/text/text_wrap.hpp"

using namespace porytiles;

namespace {

const std::string bold = "\033[1m";
const std::string reset = "\033[0m";

} // namespace

TEST(TextWrapTests, WidthZeroReturnsInputUnchanged)
{
    const std::string input = "this is a fairly long line that would otherwise wrap";
    EXPECT_EQ(wrap_ansi_line(input, 0), (std::vector<std::string>{input}));
}

TEST(TextWrapTests, ShortLineIsNotWrapped)
{
    EXPECT_EQ(wrap_ansi_line("short", 40), (std::vector<std::string>{"short"}));
}

TEST(TextWrapTests, EmptyLineStaysEmpty)
{
    EXPECT_EQ(wrap_ansi_line("", 40), (std::vector<std::string>{""}));
}

TEST(TextWrapTests, WrapsAtSpaceBoundaries)
{
    EXPECT_EQ(wrap_ansi_line("the quick brown fox", 9), (std::vector<std::string>{"the quick", "brown fox"}));
}

TEST(TextWrapTests, BreakConsumesTheSpace)
{
    // No wrapped line should start or end with the break space.
    const auto lines = wrap_ansi_line("hello world", 5);
    ASSERT_EQ(lines.size(), 2);
    EXPECT_EQ(lines[0], "hello");
    EXPECT_EQ(lines[1], "world");
}

TEST(TextWrapTests, MovesOversizedWordToNextLine)
{
    EXPECT_EQ(wrap_ansi_line("hello wor", 8), (std::vector<std::string>{"hello", "wor"}));
}

TEST(TextWrapTests, HardBreaksWordLongerThanWidth)
{
    EXPECT_EQ(wrap_ansi_line("abcdefghij", 4), (std::vector<std::string>{"abcd", "efgh", "ij"}));
}

TEST(TextWrapTests, AnsiEscapesDoNotCountTowardWidth)
{
    // Visible text is "hello" (5 cols), which fits in width 5 despite the surrounding escape bytes.
    const std::string styled = bold + "hello" + reset;
    EXPECT_EQ(wrap_ansi_line(styled, 5), (std::vector<std::string>{styled}));
}

TEST(TextWrapTests, StyleCarriesAcrossWrapBoundary)
{
    // A bold span "hello world" wrapped at 5 should close bold on the first line and re-open it on the second.
    const std::string styled = bold + "hello world" + reset;
    const auto lines = wrap_ansi_line(styled, 5);
    ASSERT_EQ(lines.size(), 2);
    EXPECT_EQ(lines[0], bold + "hello" + reset);
    EXPECT_EQ(lines[1], bold + "world" + reset);
}

TEST(TextWrapTests, DoesNotSplitMultibyteCodepoint)
{
    // "│" is U+2502, a 3-byte UTF-8 sequence counted as a single visible column.
    const std::string box = "│";
    const std::string input = box + box + box + box;
    const auto lines = wrap_ansi_line(input, 2);
    ASSERT_EQ(lines.size(), 2);
    EXPECT_EQ(lines[0], box + box);
    EXPECT_EQ(lines[1], box + box);
}
