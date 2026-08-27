#include "porytiles/utilities/c_parser/lexer.hpp"

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace porytiles {
namespace {

class LexerTests : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

    template <typename T>
    std::string get_all_error_text(const ChainableResult<T> &result)
    {
        std::string error_text;
        for (const auto &err : result.chain()) {
            auto details = err->details(formatter_);
            for (const auto &line : details) {
                error_text += line + "\n";
            }
        }
        return error_text;
    }
};

TEST_F(LexerTests, LexEmptyString)
{
    Lexer lexer{&formatter_, ""};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_TRUE(result.value()[0].is(TokenType::end_of_file));
}

TEST_F(LexerTests, LexSimpleDefine)
{
    Lexer lexer{&formatter_, "#define FOO 123"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 5);

    EXPECT_TRUE(tokens[0].is(TokenType::hash));
    EXPECT_TRUE(tokens[1].is(TokenType::kw_define));
    EXPECT_TRUE(tokens[2].is(TokenType::identifier));
    EXPECT_EQ(tokens[2].text(), "FOO");
    EXPECT_TRUE(tokens[3].is(TokenType::integer_literal));
    EXPECT_EQ(tokens[3].int_value(), 123);
    EXPECT_TRUE(tokens[4].is(TokenType::end_of_file));
}

TEST_F(LexerTests, LexHexNumber)
{
    Lexer lexer{&formatter_, "0xFF"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 2);

    EXPECT_TRUE(tokens[0].is(TokenType::integer_literal));
    EXPECT_EQ(tokens[0].int_value(), 255);
    EXPECT_EQ(tokens[0].text(), "0xFF");
}

TEST_F(LexerTests, LexBinaryNumber)
{
    Lexer lexer{&formatter_, "0b1010"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 2);

    EXPECT_TRUE(tokens[0].is(TokenType::integer_literal));
    EXPECT_EQ(tokens[0].int_value(), 10);
}

TEST_F(LexerTests, LexOctalNumber)
{
    Lexer lexer{&formatter_, "0755"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 2);

    EXPECT_TRUE(tokens[0].is(TokenType::integer_literal));
    EXPECT_EQ(tokens[0].int_value(), 493); // 0755 octal = 493 decimal
}

TEST_F(LexerTests, LexStringLiteral)
{
    Lexer lexer{&formatter_, "\"hello world\""};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 2);

    EXPECT_TRUE(tokens[0].is(TokenType::string_literal));
    EXPECT_EQ(tokens[0].text(), "hello world");
}

TEST_F(LexerTests, LexStringWithEscapes)
{
    Lexer lexer{&formatter_, "\"line1\\nline2\""};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 2);

    EXPECT_TRUE(tokens[0].is(TokenType::string_literal));
    EXPECT_EQ(tokens[0].text(), "line1\nline2");
}

TEST_F(LexerTests, LexOperators)
{
    Lexer lexer{&formatter_, "+ - * / % & | ^ ~ ! < > ="};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_TRUE(tokens[0].is(TokenType::plus));
    EXPECT_TRUE(tokens[1].is(TokenType::minus));
    EXPECT_TRUE(tokens[2].is(TokenType::star));
    EXPECT_TRUE(tokens[3].is(TokenType::slash));
    EXPECT_TRUE(tokens[4].is(TokenType::percent));
    EXPECT_TRUE(tokens[5].is(TokenType::ampersand));
    EXPECT_TRUE(tokens[6].is(TokenType::pipe));
    EXPECT_TRUE(tokens[7].is(TokenType::caret));
    EXPECT_TRUE(tokens[8].is(TokenType::tilde));
    EXPECT_TRUE(tokens[9].is(TokenType::exclaim));
    EXPECT_TRUE(tokens[10].is(TokenType::less));
    EXPECT_TRUE(tokens[11].is(TokenType::greater));
    EXPECT_TRUE(tokens[12].is(TokenType::equal));
}

TEST_F(LexerTests, LexCompoundOperators)
{
    Lexer lexer{&formatter_, "<< >> && || == != <= >="};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_TRUE(tokens[0].is(TokenType::less_less));
    EXPECT_TRUE(tokens[1].is(TokenType::greater_greater));
    EXPECT_TRUE(tokens[2].is(TokenType::ampersand_ampersand));
    EXPECT_TRUE(tokens[3].is(TokenType::pipe_pipe));
    EXPECT_TRUE(tokens[4].is(TokenType::equal_equal));
    EXPECT_TRUE(tokens[5].is(TokenType::exclaim_equal));
    EXPECT_TRUE(tokens[6].is(TokenType::less_equal));
    EXPECT_TRUE(tokens[7].is(TokenType::greater_equal));
}

TEST_F(LexerTests, LexDelimiters)
{
    Lexer lexer{&formatter_, "( ) { } [ ] , ;"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_TRUE(tokens[0].is(TokenType::left_paren));
    EXPECT_TRUE(tokens[1].is(TokenType::right_paren));
    EXPECT_TRUE(tokens[2].is(TokenType::left_brace));
    EXPECT_TRUE(tokens[3].is(TokenType::right_brace));
    EXPECT_TRUE(tokens[4].is(TokenType::left_bracket));
    EXPECT_TRUE(tokens[5].is(TokenType::right_bracket));
    EXPECT_TRUE(tokens[6].is(TokenType::comma));
    EXPECT_TRUE(tokens[7].is(TokenType::semicolon));
}

TEST_F(LexerTests, LexNewlines)
{
    Lexer lexer{&formatter_, "a\nb\nc"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_TRUE(tokens[0].is(TokenType::identifier));
    EXPECT_TRUE(tokens[1].is(TokenType::newline));
    EXPECT_TRUE(tokens[2].is(TokenType::identifier));
    EXPECT_TRUE(tokens[3].is(TokenType::newline));
    EXPECT_TRUE(tokens[4].is(TokenType::identifier));
}

TEST_F(LexerTests, LexLineComment)
{
    Lexer lexer{&formatter_, "foo // this is a comment\nbar"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_TRUE(tokens[0].is(TokenType::identifier));
    EXPECT_EQ(tokens[0].text(), "foo");
    EXPECT_TRUE(tokens[1].is(TokenType::newline));
    EXPECT_TRUE(tokens[2].is(TokenType::identifier));
    EXPECT_EQ(tokens[2].text(), "bar");
}

TEST_F(LexerTests, LexBlockComment)
{
    Lexer lexer{&formatter_, "foo /* comment */ bar"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_TRUE(tokens[0].is(TokenType::identifier));
    EXPECT_EQ(tokens[0].text(), "foo");
    EXPECT_TRUE(tokens[1].is(TokenType::identifier));
    EXPECT_EQ(tokens[1].text(), "bar");
}

TEST_F(LexerTests, LexMultilineBlockComment)
{
    Lexer lexer{&formatter_, "foo /* line1\nline2 */ bar"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_TRUE(tokens[0].is(TokenType::identifier));
    EXPECT_TRUE(tokens[1].is(TokenType::identifier));
    EXPECT_EQ(tokens[1].text(), "bar");
}

TEST_F(LexerTests, LexKeywords)
{
    Lexer lexer{&formatter_, "define ifdef ifndef endif include"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_TRUE(tokens[0].is(TokenType::kw_define));
    EXPECT_TRUE(tokens[1].is(TokenType::kw_ifdef));
    EXPECT_TRUE(tokens[2].is(TokenType::kw_ifndef));
    EXPECT_TRUE(tokens[3].is(TokenType::kw_endif));
    EXPECT_TRUE(tokens[4].is(TokenType::kw_include));
}

TEST_F(LexerTests, LexEnumKeyword)
{
    Lexer lexer{&formatter_, "enum"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value()[0].is(TokenType::kw_enum));
}

TEST_F(LexerTests, LexSourcePosition)
{
    Lexer lexer{&formatter_, "foo\nbar"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_EQ(tokens[0].position().line, 1);
    EXPECT_EQ(tokens[0].position().column, 1);
    EXPECT_EQ(tokens[2].position().line, 2);
    EXPECT_EQ(tokens[2].position().column, 1);
}

TEST_F(LexerTests, UnterminatedStringReturnsError)
{
    Lexer lexer{&formatter_, "\"unterminated"};
    auto result = lexer.lex();
    EXPECT_FALSE(result.has_value());
}

TEST_F(LexerTests, UnterminatedStringErrorPosition)
{
    Lexer lexer{&formatter_, "\"unterminated"};
    auto result = lexer.lex();
    ASSERT_FALSE(result.has_value());
    ASSERT_FALSE(result.chain().empty());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("1:1:"), std::string::npos);
    EXPECT_NE(error_text.find("unterminated string literal"), std::string::npos);
}

TEST_F(LexerTests, UnterminatedStringErrorPositionOnLine3)
{
    Lexer lexer{&formatter_, "foo\nbar\n\"unterminated"};
    auto result = lexer.lex();
    ASSERT_FALSE(result.has_value());
    ASSERT_FALSE(result.chain().empty());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("3:1:"), std::string::npos);
}

TEST_F(LexerTests, UnterminatedBlockCommentReturnsError)
{
    Lexer lexer{&formatter_, "/* unterminated"};
    auto result = lexer.lex();
    EXPECT_FALSE(result.has_value());
}

TEST_F(LexerTests, UnterminatedBlockCommentErrorPosition)
{
    Lexer lexer{&formatter_, "/* unterminated"};
    auto result = lexer.lex();
    ASSERT_FALSE(result.has_value());
    ASSERT_FALSE(result.chain().empty());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("1:1:"), std::string::npos);
    EXPECT_NE(error_text.find("unterminated block comment"), std::string::npos);
}

TEST_F(LexerTests, UnterminatedBlockCommentErrorPositionOnLine2)
{
    Lexer lexer{&formatter_, "foo\n/* unterminated"};
    auto result = lexer.lex();
    ASSERT_FALSE(result.has_value());
    ASSERT_FALSE(result.chain().empty());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("2:1:"), std::string::npos);
}

TEST_F(LexerTests, InvalidHexLiteralErrorPosition)
{
    Lexer lexer{&formatter_, "0x"};
    auto result = lexer.lex();
    ASSERT_FALSE(result.has_value());
    ASSERT_FALSE(result.chain().empty());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("1:1:"), std::string::npos);
    EXPECT_NE(error_text.find("invalid hexadecimal literal '0x'"), std::string::npos);
}

TEST_F(LexerTests, InvalidBinaryLiteralErrorPosition)
{
    Lexer lexer{&formatter_, "0b"};
    auto result = lexer.lex();
    ASSERT_FALSE(result.has_value());
    ASSERT_FALSE(result.chain().empty());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("1:1:"), std::string::npos);
    EXPECT_NE(error_text.find("invalid binary literal '0b'"), std::string::npos);
}

TEST_F(LexerTests, UnterminatedStringErrorPositionMidLine)
{
    Lexer lexer{&formatter_, "foo \"unterminated"};
    auto result = lexer.lex();
    ASSERT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("1:5:"), std::string::npos); // "foo " is 4 chars, quote starts at column 5
    EXPECT_NE(error_text.find("unterminated string literal"), std::string::npos);
}

TEST_F(LexerTests, UnterminatedBlockCommentErrorPositionMidLine)
{
    Lexer lexer{&formatter_, "baz /* unterminated"};
    auto result = lexer.lex();
    ASSERT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("1:5:"), std::string::npos); // "baz " is 4 chars, /* starts at column 5
    EXPECT_NE(error_text.find("unterminated block comment"), std::string::npos);
}

TEST_F(LexerTests, InvalidHexLiteralErrorPositionMidLine)
{
    Lexer lexer{&formatter_, "foo 0x"};
    auto result = lexer.lex();
    ASSERT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("1:5:"), std::string::npos); // "foo " is 4 chars, 0x starts at column 5
    EXPECT_NE(error_text.find("invalid hexadecimal literal '0x'"), std::string::npos);
}

TEST_F(LexerTests, InvalidBinaryLiteralErrorPositionMidLine)
{
    Lexer lexer{&formatter_, "bar 0b"};
    auto result = lexer.lex();
    ASSERT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("1:5:"), std::string::npos); // "bar " is 4 chars, 0b starts at column 5
    EXPECT_NE(error_text.find("invalid binary literal '0b'"), std::string::npos);
}

TEST_F(LexerTests, LexCompleteDefineExpression)
{
    Lexer lexer{&formatter_, "#define BAZ (1 << 4)"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_TRUE(tokens[0].is(TokenType::hash));
    EXPECT_TRUE(tokens[1].is(TokenType::kw_define));
    EXPECT_TRUE(tokens[2].is(TokenType::identifier));
    EXPECT_EQ(tokens[2].text(), "BAZ");
    EXPECT_TRUE(tokens[3].is(TokenType::left_paren));
    EXPECT_TRUE(tokens[4].is(TokenType::integer_literal));
    EXPECT_EQ(tokens[4].int_value(), 1);
    EXPECT_TRUE(tokens[5].is(TokenType::less_less));
    EXPECT_TRUE(tokens[6].is(TokenType::integer_literal));
    EXPECT_EQ(tokens[6].int_value(), 4);
    EXPECT_TRUE(tokens[7].is(TokenType::right_paren));
}

TEST_F(LexerTests, LexNumberWithSuffix)
{
    Lexer lexer{&formatter_, "123U 456L 789ULL"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_TRUE(tokens[0].is(TokenType::integer_literal));
    EXPECT_EQ(tokens[0].int_value(), 123);
    EXPECT_TRUE(tokens[1].is(TokenType::integer_literal));
    EXPECT_EQ(tokens[1].int_value(), 456);
    EXPECT_TRUE(tokens[2].is(TokenType::integer_literal));
    EXPECT_EQ(tokens[2].int_value(), 789);
}

TEST_F(LexerTests, LineContinuationSplicesValue)
{
    // A backslash immediately before a newline splices the two physical lines, so no newline token separates the
    // continued value from the macro name.
    Lexer lexer{&formatter_, "#define FLAGS (FOO | \\\n                      BAR)"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    for (const auto &token : tokens) {
        EXPECT_FALSE(token.is(TokenType::newline));
        EXPECT_FALSE(token.is(TokenType::unknown));
    }
    ASSERT_GE(tokens.size(), 9);
    EXPECT_TRUE(tokens[0].is(TokenType::hash));
    EXPECT_TRUE(tokens[2].is(TokenType::identifier));
    EXPECT_EQ(tokens[2].text(), "FLAGS");
    EXPECT_TRUE(tokens[3].is(TokenType::left_paren));
    EXPECT_EQ(tokens[4].text(), "FOO");
    EXPECT_TRUE(tokens[5].is(TokenType::pipe));
    EXPECT_EQ(tokens[6].text(), "BAR");
    EXPECT_TRUE(tokens[7].is(TokenType::right_paren));
}

TEST_F(LexerTests, LineContinuationTracksLineNumber)
{
    // After splicing, the lexer must still count the physical newline so positions on the next line stay correct.
    Lexer lexer{&formatter_, "A \\\nB"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].text(), "A");
    EXPECT_EQ(tokens[0].position().line, 1U);
    EXPECT_EQ(tokens[1].text(), "B");
    EXPECT_EQ(tokens[1].position().line, 2U);
}

TEST_F(LexerTests, LineContinuationHandlesCrlf)
{
    // A backslash before a Windows CRLF newline also splices without emitting a token.
    Lexer lexer{&formatter_, "X \\\r\nY"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    for (const auto &token : tokens) {
        EXPECT_FALSE(token.is(TokenType::newline));
        EXPECT_FALSE(token.is(TokenType::unknown));
    }
    ASSERT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].text(), "X");
    EXPECT_EQ(tokens[1].text(), "Y");
}

TEST_F(LexerTests, BackslashNotBeforeNewlineStaysUnknown)
{
    // A backslash that is not immediately before a newline keeps the previous unknown-token behavior.
    Lexer lexer{&formatter_, "\\x"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 1);
    EXPECT_TRUE(tokens[0].is(TokenType::unknown));
}

} // namespace
} // namespace porytiles
