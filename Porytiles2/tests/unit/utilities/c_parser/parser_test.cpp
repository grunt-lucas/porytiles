#include "porytiles2/utilities/c_parser/parser.hpp"

#include <gtest/gtest.h>

#include "porytiles2/utilities/c_parser/lexer.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

namespace porytiles2 {
namespace {

class ParserTests : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

    [[nodiscard]] ChainableResult<std::vector<DefineStatement>> parse(const std::string &source)
    {
        Lexer lexer{source};
        auto tokens_result = lexer.lex();
        if (!tokens_result.has_value()) {
            return ChainableResult<std::vector<DefineStatement>>{tokens_result};
        }
        Parser parser{std::move(tokens_result).value()};
        return parser.parse_defines();
    }
};

TEST_F(ParserTests, ParseEmptyString)
{
    auto result = parse("");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(ParserTests, ParseSimpleDefine)
{
    auto result = parse("#define FOO 123");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].name(), "FOO");
    EXPECT_TRUE(defines[0].has_int_value());
    EXPECT_EQ(defines[0].int_value(), 123);
}

TEST_F(ParserTests, ParseHexDefine)
{
    auto result = parse("#define BAR 0xFF");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].name(), "BAR");
    EXPECT_TRUE(defines[0].has_int_value());
    EXPECT_EQ(defines[0].int_value(), 255);
}

TEST_F(ParserTests, ParseBinaryDefine)
{
    auto result = parse("#define BIN 0b1010");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].name(), "BIN");
    EXPECT_TRUE(defines[0].has_int_value());
    EXPECT_EQ(defines[0].int_value(), 10);
}

TEST_F(ParserTests, ParseFlagDefine)
{
    auto result = parse("#define DEBUG\n");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].name(), "DEBUG");
    EXPECT_TRUE(defines[0].is_flag());
}

TEST_F(ParserTests, ParseStringDefine)
{
    auto result = parse("#define MSG \"hello world\"\n");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].name(), "MSG");
    EXPECT_TRUE(defines[0].has_string_value());
    EXPECT_EQ(defines[0].string_value(), "hello world");
}

TEST_F(ParserTests, ParseMultipleDefines)
{
    auto result = parse("#define A 1\n#define B 2\n#define C 3");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 3);
    EXPECT_EQ(defines[0].name(), "A");
    EXPECT_EQ(defines[0].int_value(), 1);
    EXPECT_EQ(defines[1].name(), "B");
    EXPECT_EQ(defines[1].int_value(), 2);
    EXPECT_EQ(defines[2].name(), "C");
    EXPECT_EQ(defines[2].int_value(), 3);
}

TEST_F(ParserTests, ParseAdditionExpression)
{
    auto result = parse("#define SUM 1 + 2 + 3");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 6);
}

TEST_F(ParserTests, ParseSubtractionExpression)
{
    auto result = parse("#define DIFF 10 - 3");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 7);
}

TEST_F(ParserTests, ParseMultiplicationExpression)
{
    auto result = parse("#define PROD 4 * 5");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 20);
}

TEST_F(ParserTests, ParseDivisionExpression)
{
    auto result = parse("#define QUOT 20 / 4");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 5);
}

TEST_F(ParserTests, ParseModuloExpression)
{
    auto result = parse("#define MOD 17 % 5");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 2);
}

TEST_F(ParserTests, ParseOperatorPrecedence)
{
    // 1 + 2 * 3 should be 7, not 9
    auto result = parse("#define PREC 1 + 2 * 3");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 7);
}

TEST_F(ParserTests, ParseParenthesizedExpression)
{
    // (1 + 2) * 3 should be 9
    auto result = parse("#define PAREN (1 + 2) * 3");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 9);
}

TEST_F(ParserTests, ParseLeftShift)
{
    auto result = parse("#define SHIFT 1 << 4");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 16);
}

TEST_F(ParserTests, ParseRightShift)
{
    auto result = parse("#define RSHIFT 16 >> 2");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 4);
}

TEST_F(ParserTests, ParseBitwiseAnd)
{
    auto result = parse("#define BAND 0xFF & 0x0F");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 0x0F);
}

TEST_F(ParserTests, ParseBitwiseOr)
{
    auto result = parse("#define BOR 0xF0 | 0x0F");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 0xFF);
}

TEST_F(ParserTests, ParseBitwiseXor)
{
    auto result = parse("#define BXOR 0xFF ^ 0x0F");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 0xF0);
}

TEST_F(ParserTests, ParseUnaryMinus)
{
    auto result = parse("#define NEG -5");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), -5);
}

TEST_F(ParserTests, ParseUnaryNot)
{
    auto result = parse("#define NOT ~0");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), ~static_cast<std::int64_t>(0));
}

TEST_F(ParserTests, ParseComplexExpression)
{
    // ((1 << 4) | (1 << 2)) = 16 | 4 = 20
    auto result = parse("#define COMPLEX ((1 << 4) | (1 << 2))");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].int_value(), 20);
}

TEST_F(ParserTests, ParseMacroReference)
{
    auto result = parse("#define A 10\n#define B A + 5");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 2);
    EXPECT_EQ(defines[0].name(), "A");
    EXPECT_EQ(defines[0].int_value(), 10);
    EXPECT_EQ(defines[1].name(), "B");
    EXPECT_EQ(defines[1].int_value(), 15);
}

TEST_F(ParserTests, ParseChainedMacroReferences)
{
    auto result = parse("#define A 1\n#define B A + 1\n#define C B + 1");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 3);
    EXPECT_EQ(defines[0].int_value(), 1);
    EXPECT_EQ(defines[1].int_value(), 2);
    EXPECT_EQ(defines[2].int_value(), 3);
}

TEST_F(ParserTests, ParseSkipsOtherPreprocessorDirectives)
{
    auto result = parse("#ifdef FOO\n#define BAR 123\n#endif");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].name(), "BAR");
    EXPECT_EQ(defines[0].int_value(), 123);
}

TEST_F(ParserTests, ParseParametricMacroAsFlag)
{
    // Parametric macros should be treated as flag defines
    auto result = parse("#define MAX(a, b) ((a) > (b) ? (a) : (b))");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 1);
    EXPECT_EQ(defines[0].name(), "MAX");
    EXPECT_TRUE(defines[0].is_flag());
}

TEST_F(ParserTests, ParseMixedContent)
{
    auto result = parse(R"(
// Comment
#define FOO 1
int some_code;
#define BAR 2
/* block comment */
#define BAZ 3
)");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 3);
    EXPECT_EQ(defines[0].name(), "FOO");
    EXPECT_EQ(defines[1].name(), "BAR");
    EXPECT_EQ(defines[2].name(), "BAZ");
}

TEST_F(ParserTests, DivisionByZeroReturnsError)
{
    auto result = parse("#define DIV 10 / 0");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ParserTests, ModuloByZeroReturnsError)
{
    auto result = parse("#define MOD 10 % 0");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ParserTests, UnknownIdentifierReturnsError)
{
    auto result = parse("#define FOO UNKNOWN_MACRO");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ParserTests, ParseRealWorldExample)
{
    auto result = parse(R"(
#define TILE_SIZE 8
#define TILES_PER_METATILE 4
#define METATILE_SIZE (TILE_SIZE * TILES_PER_METATILE)
#define MAX_PALETTES 16
#define PALETTE_MASK 0x0F
)");
    ASSERT_TRUE(result.has_value());

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 5);

    EXPECT_EQ(defines[0].name(), "TILE_SIZE");
    EXPECT_EQ(defines[0].int_value(), 8);

    EXPECT_EQ(defines[1].name(), "TILES_PER_METATILE");
    EXPECT_EQ(defines[1].int_value(), 4);

    EXPECT_EQ(defines[2].name(), "METATILE_SIZE");
    EXPECT_EQ(defines[2].int_value(), 32); // 8 * 4

    EXPECT_EQ(defines[3].name(), "MAX_PALETTES");
    EXPECT_EQ(defines[3].int_value(), 16);

    EXPECT_EQ(defines[4].name(), "PALETTE_MASK");
    EXPECT_EQ(defines[4].int_value(), 0x0F);
}

} // namespace
} // namespace porytiles2
