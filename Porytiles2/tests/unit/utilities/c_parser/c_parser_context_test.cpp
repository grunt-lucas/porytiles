#include "porytiles2/utilities/c_parser/c_parser_context.hpp"

#include <gtest/gtest.h>

#include "porytiles2/utilities/text/plain_text_formatter.hpp"

namespace porytiles2 {
namespace {

class CParserContextTests : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;
};

TEST_F(CParserContextTests, MakeErrorWithFilePath)
{
    std::vector<std::string> lines = {"#define FOO 123"};
    CParserContext context{&lines, &formatter_, "test.h"};

    auto error = context.make_error({1, 9}, "test error message");

    std::string error_text = error.join(formatter_);
    // Should contain file path, line, column, and message
    EXPECT_NE(error_text.find("test.h:1:9:"), std::string::npos);
    EXPECT_NE(error_text.find("test error message"), std::string::npos);
}

TEST_F(CParserContextTests, MakeErrorWithoutFilePath)
{
    std::vector<std::string> lines = {"#define FOO 123"};
    CParserContext context{&lines, &formatter_};

    auto error = context.make_error({1, 9}, "test error message");

    std::string error_text = error.join(formatter_);
    // Should contain line:col: format without file path
    EXPECT_NE(error_text.find("1:9:"), std::string::npos);
    EXPECT_NE(error_text.find("test error message"), std::string::npos);
}

TEST_F(CParserContextTests, MakeErrorIncludesSourceContext)
{
    std::vector<std::string> lines = {"#define FOO 123", "#define BAR 456", "#define BAZ 789"};
    CParserContext context{&lines, &formatter_, "test.h"};

    auto error = context.make_error({2, 13}, "some error");

    std::string error_text = error.join(formatter_);
    // Should include surrounding lines
    EXPECT_NE(error_text.find("#define FOO 123"), std::string::npos);
    EXPECT_NE(error_text.find("#define BAR 456"), std::string::npos);
    EXPECT_NE(error_text.find("#define BAZ 789"), std::string::npos);
}

TEST_F(CParserContextTests, MakeErrorWithColumnHighlight)
{
    std::vector<std::string> lines = {"#define BAR UNDEFINED"};
    CParserContext context{&lines, &formatter_, "test.h"};

    auto error = context.make_error({1, 13}, "unknown identifier");

    // The error should have multiple lines (header + source + caret)
    auto details = error.details(formatter_);
    EXPECT_GE(details.size(), 3); // Header, source line, caret line at minimum
}

TEST_F(CParserContextTests, MakeErrorIncludesCaretIndicator)
{
    std::vector<std::string> lines = {"#define BAR UNDEFINED"};
    CParserContext context{&lines, &formatter_, "test.h"};

    auto error = context.make_error({1, 13}, "unknown identifier");

    std::string error_text = error.join(formatter_);
    // Should include the caret indicator
    EXPECT_NE(error_text.find("^"), std::string::npos);
}

TEST_F(CParserContextTests, MakeErrorHandlesLineOutOfBounds)
{
    std::vector<std::string> lines = {"#define FOO 123"};
    CParserContext context{&lines, &formatter_, "test.h"};

    // Line 5 doesn't exist in a 1-line file
    auto error = context.make_error({5, 1}, "out of bounds error");

    std::string error_text = error.join(formatter_);
    // Should still have the header with position info
    EXPECT_NE(error_text.find("test.h:5:1:"), std::string::npos);
    EXPECT_NE(error_text.find("out of bounds error"), std::string::npos);
    // But should NOT include source context since line is out of bounds
    EXPECT_EQ(error_text.find("#define FOO"), std::string::npos);
}

TEST_F(CParserContextTests, MakeErrorHandlesColumnOutOfBounds)
{
    std::vector<std::string> lines = {"abc"};
    CParserContext context{&lines, &formatter_, "test.h"};

    // Column 50 is beyond the 3-character line
    auto error = context.make_error({1, 50}, "column out of bounds");

    std::string error_text = error.join(formatter_);
    // Should still have header info
    EXPECT_NE(error_text.find("test.h:1:50:"), std::string::npos);
    // Line should still be included (highlighted without column caret)
    EXPECT_NE(error_text.find("abc"), std::string::npos);
}

TEST_F(CParserContextTests, MakeErrorHandlesEmptyLine)
{
    std::vector<std::string> lines = {"abc", "", "def"};
    CParserContext context{&lines, &formatter_, "test.h"};

    // Line 2 is empty
    auto error = context.make_error({2, 1}, "error on empty line");

    std::string error_text = error.join(formatter_);
    EXPECT_NE(error_text.find("test.h:2:1:"), std::string::npos);
}

TEST_F(CParserContextTests, MakeErrorHandlesZeroLine)
{
    std::vector<std::string> lines = {"#define FOO 123"};
    CParserContext context{&lines, &formatter_, "test.h"};

    // Line 0 is invalid (1-based)
    auto error = context.make_error({0, 1}, "line zero error");

    std::string error_text = error.join(formatter_);
    // Should have header but no source context
    EXPECT_NE(error_text.find("test.h:0:1:"), std::string::npos);
}

TEST_F(CParserContextTests, AccessorFileLinesReturnsPointer)
{
    std::vector<std::string> lines = {"test"};
    CParserContext context{&lines, &formatter_, "test.h"};

    EXPECT_EQ(context.file_lines(), &lines);
}

TEST_F(CParserContextTests, AccessorFormatterReturnsPointer)
{
    std::vector<std::string> lines = {"test"};
    CParserContext context{&lines, &formatter_, "test.h"};

    EXPECT_EQ(context.formatter(), &formatter_);
}

TEST_F(CParserContextTests, AccessorFilePathReturnsPath)
{
    std::vector<std::string> lines = {"test"};
    CParserContext context{&lines, &formatter_, "test.h"};

    EXPECT_EQ(context.file_path(), "test.h");
}

TEST_F(CParserContextTests, AccessorFilePathEmptyWhenNotProvided)
{
    std::vector<std::string> lines = {"test"};
    CParserContext context{&lines, &formatter_};

    EXPECT_TRUE(context.file_path().empty());
}

TEST_F(CParserContextTests, MakeErrorMultipleLines)
{
    std::vector<std::string> lines = {
        "#define A 1", "#define B 2", "#define C 3", "#define D 4", "#define E 5", "#define F 6", "#define G 7"};
    CParserContext context{&lines, &formatter_, "test.h"};

    // Error on line 4, should show window around it
    auto error = context.make_error({4, 9}, "middle error");

    std::string error_text = error.join(formatter_);
    // Should include lines around line 4 (with window_size of 5)
    EXPECT_NE(error_text.find("#define D 4"), std::string::npos);
}

} // namespace
} // namespace porytiles2
