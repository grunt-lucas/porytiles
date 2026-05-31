#include "gtest/gtest.h"

#include <cstddef>
#include <string>
#include <vector>

#include "porytiles/utilities/text/file_highlight_printer.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

using namespace porytiles;

class FileHighlightPrinterTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;
    FileHighlightPrinter printer_{&formatter_};

    std::vector<std::string> sample_lines_{"line 0", "line 1", "line 2", "line 3", "line 4"};
};

TEST_F(FileHighlightPrinterTest, EmptyLinesReturnsEmptyResult)
{
    std::vector<std::string> empty_lines;
    auto result = printer_.print(empty_lines, std::vector<std::size_t>{0}, 5);

    EXPECT_TRUE(result.empty());
}

TEST_F(FileHighlightPrinterTest, EmptyHighlightIndicesReturnsEmptyResult)
{
    auto result = printer_.print(sample_lines_, std::vector<std::size_t>{}, 5);

    EXPECT_TRUE(result.empty());
}

TEST_F(FileHighlightPrinterTest, SingleLineHighlightedInMiddle)
{
    auto result = printer_.print(sample_lines_, std::vector<std::size_t>{2}, 3);

    // Window of 3 around line 2 (0-indexed) should show lines 1, 2, 3
    ASSERT_EQ(result.size(), 3);
    EXPECT_NE(result[0].find("line 1"), std::string::npos);
    EXPECT_NE(result[1].find("line 2"), std::string::npos);
    EXPECT_NE(result[2].find("line 3"), std::string::npos);
}

TEST_F(FileHighlightPrinterTest, HighlightedLineHasArrowPrefix)
{
    auto result = printer_.print(sample_lines_, std::vector<std::size_t>{2}, 3);

    ASSERT_GE(result.size(), 2);
    EXPECT_NE(result[1].find("➞"), std::string::npos);

    // Other lines should NOT have arrow
    EXPECT_EQ(result[0].find("➞"), std::string::npos);
    EXPECT_EQ(result[2].find("➞"), std::string::npos);
}

TEST_F(FileHighlightPrinterTest, LineNumbersAreOneBased)
{
    auto result = printer_.print(sample_lines_, std::vector<std::size_t>{2}, 3);

    // Should show 1-based line numbers: 2, 3, 4
    ASSERT_EQ(result.size(), 3);
    EXPECT_NE(result[0].find("2:"), std::string::npos);
    EXPECT_NE(result[1].find("3:"), std::string::npos);
    EXPECT_NE(result[2].find("4:"), std::string::npos);
}

TEST_F(FileHighlightPrinterTest, MultipleHighlightedLines)
{
    auto result = printer_.print(sample_lines_, std::vector<std::size_t>{1, 3}, 5);

    std::size_t arrow_count = 0;
    for (const auto &line : result) {
        if (line.find("➞") != std::string::npos) {
            ++arrow_count;
        }
    }
    EXPECT_EQ(arrow_count, 2);
}

TEST_F(FileHighlightPrinterTest, WindowAtStartOfFile)
{
    auto result = printer_.print(sample_lines_, std::vector<std::size_t>{0}, 3);

    // Highlighting line 0 with window 3 should show lines 0, 1
    ASSERT_GE(result.size(), 1);
    EXPECT_NE(result[0].find("1:"), std::string::npos); // Line 1 (1-indexed)
    EXPECT_NE(result[0].find("line 0"), std::string::npos);
}

TEST_F(FileHighlightPrinterTest, WindowAtEndOfFile)
{
    auto result = printer_.print(sample_lines_, std::vector<std::size_t>{4}, 3);

    // Highlighting line 4 (last) with window 3 should show lines 3, 4, 5
    ASSERT_GE(result.size(), 1);

    EXPECT_NE(result.back().find("line 4"), std::string::npos);
}

TEST_F(FileHighlightPrinterTest, ColumnHighlightEmptyLinesReturnsEmptyResult)
{
    std::vector<std::string> empty_lines;
    auto result = printer_.print(empty_lines, 0, 0, 5);

    EXPECT_TRUE(result.empty());
}

TEST_F(FileHighlightPrinterTest, ColumnHighlightIncludesCaretIndicator)
{
    std::vector<std::string> lines{"abcdef"};
    auto result = printer_.print(lines, 0, 3, 3);

    ASSERT_EQ(result.size(), 2);
    EXPECT_NE(result[1].find("^"), std::string::npos);
}

TEST_F(FileHighlightPrinterTest, ColumnHighlightHasArrowPrefix)
{
    std::vector<std::string> lines{"abcdef"};
    auto result = printer_.print(lines, 0, 3, 3);

    ASSERT_GE(result.size(), 1);
    EXPECT_NE(result[0].find("➞"), std::string::npos);
}

TEST_F(FileHighlightPrinterTest, ColumnHighlightAtStart)
{
    std::vector<std::string> lines{"abcdef"};
    auto result = printer_.print(lines, 0, 0, 3);

    // Caret should be positioned at column 0 (after prefix/line number)
    ASSERT_EQ(result.size(), 2);
    EXPECT_NE(result[1].find("^"), std::string::npos);
}

TEST_F(FileHighlightPrinterTest, ColumnHighlightAtEnd)
{
    std::vector<std::string> lines{"abcdef"};
    auto result = printer_.print(lines, 0, 5, 3); // Last char 'f'

    ASSERT_EQ(result.size(), 2);
    EXPECT_NE(result[1].find("^"), std::string::npos);
}

TEST_F(FileHighlightPrinterTest, ColumnHighlightWithContextLines)
{
    auto result = printer_.print(sample_lines_, 2, 0, 5);

    // Should have context lines + highlighted line + caret line
    // Window of 5 around line 2: lines 0,1,2,3,4 plus caret = 6 lines total
    EXPECT_GE(result.size(), 3); // At minimum: before, highlighted, caret

    // Count arrows (should be exactly 1)
    std::size_t arrow_count = 0;
    for (const auto &line : result) {
        if (line.find("➞") != std::string::npos) {
            ++arrow_count;
        }
    }
    EXPECT_EQ(arrow_count, 1);
}

TEST_F(FileHighlightPrinterTest, CaretLineFollowsHighlightedLine)
{
    auto result = printer_.print(sample_lines_, 2, 0, 5);

    // Find the highlighted line (with arrow)
    std::size_t highlighted_idx = 0;
    for (std::size_t i = 0; i < result.size(); ++i) {
        if (result[i].find("➞") != std::string::npos) {
            highlighted_idx = i;
            break;
        }
    }

    // The line immediately after should be the caret line
    ASSERT_LT(highlighted_idx + 1, result.size());
    EXPECT_NE(result[highlighted_idx + 1].find("^"), std::string::npos);
}
