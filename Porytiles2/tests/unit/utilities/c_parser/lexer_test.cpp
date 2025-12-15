#include "porytiles2/utilities/c_parser/lexer.hpp"

#include <gtest/gtest.h>

#include "porytiles2/utilities/text/plain_text_formatter.hpp"

namespace porytiles2 {
namespace {

class LexerTests : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;
};

TEST_F(LexerTests, LexEmptyString)
{
    Lexer lexer{""};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_TRUE(result.value()[0].is(TokenType::end_of_file));
}

TEST_F(LexerTests, LexSimpleDefine)
{
    Lexer lexer{"#define FOO 123"};
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
    Lexer lexer{"0xFF"};
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
    Lexer lexer{"0b1010"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 2);

    EXPECT_TRUE(tokens[0].is(TokenType::integer_literal));
    EXPECT_EQ(tokens[0].int_value(), 10);
}

TEST_F(LexerTests, LexOctalNumber)
{
    Lexer lexer{"0755"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 2);

    EXPECT_TRUE(tokens[0].is(TokenType::integer_literal));
    EXPECT_EQ(tokens[0].int_value(), 493); // 0755 octal = 493 decimal
}

TEST_F(LexerTests, LexStringLiteral)
{
    Lexer lexer{"\"hello world\""};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 2);

    EXPECT_TRUE(tokens[0].is(TokenType::string_literal));
    EXPECT_EQ(tokens[0].text(), "hello world");
}

TEST_F(LexerTests, LexStringWithEscapes)
{
    Lexer lexer{"\"line1\\nline2\""};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    ASSERT_GE(tokens.size(), 2);

    EXPECT_TRUE(tokens[0].is(TokenType::string_literal));
    EXPECT_EQ(tokens[0].text(), "line1\nline2");
}

TEST_F(LexerTests, LexOperators)
{
    Lexer lexer{"+ - * / % & | ^ ~ ! < > ="};
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
    Lexer lexer{"<< >> && || == != <= >="};
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
    Lexer lexer{"( ) { } [ ] , ;"};
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
    Lexer lexer{"a\nb\nc"};
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
    Lexer lexer{"foo // this is a comment\nbar"};
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
    Lexer lexer{"foo /* comment */ bar"};
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
    Lexer lexer{"foo /* line1\nline2 */ bar"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());

    const auto &tokens = result.value();
    EXPECT_TRUE(tokens[0].is(TokenType::identifier));
    EXPECT_TRUE(tokens[1].is(TokenType::identifier));
    EXPECT_EQ(tokens[1].text(), "bar");
}

TEST_F(LexerTests, LexKeywords)
{
    Lexer lexer{"define ifdef ifndef endif include"};
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
    Lexer lexer{"enum"};
    auto result = lexer.lex();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value()[0].is(TokenType::kw_enum));
}

TEST_F(LexerTests, LexSourcePosition)
{
    Lexer lexer{"foo\nbar"};
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
    Lexer lexer{"\"unterminated"};
    auto result = lexer.lex();
    EXPECT_FALSE(result.has_value());
}

TEST_F(LexerTests, UnterminatedBlockCommentReturnsError)
{
    Lexer lexer{"/* unterminated"};
    auto result = lexer.lex();
    EXPECT_FALSE(result.has_value());
}

TEST_F(LexerTests, LexCompleteDefineExpression)
{
    Lexer lexer{"#define BAZ (1 << 4)"};
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
    Lexer lexer{"123U 456L 789ULL"};
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

} // namespace
} // namespace porytiles2
