#include "porytiles2/utilities/c_parser/parser.hpp"

#include <gtest/gtest.h>

#include "porytiles2/utilities/c_parser/lexer.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

namespace porytiles2 {
namespace {

class ParserTests : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

    [[nodiscard]] ChainableResult<std::vector<DefineStatement>, CParserError> parse(const std::string &source)
    {
        Lexer lexer{source};
        auto tokens_result = lexer.lex();
        if (!tokens_result.has_value()) {
            return ChainableResult<std::vector<DefineStatement>, CParserError>{tokens_result};
        }
        Parser parser{std::move(tokens_result).value()};
        return parser.parse_defines();
    }

    [[nodiscard]] ChainableResult<std::vector<EnumDeclaration>, CParserError> parse_enums(const std::string &source)
    {
        Lexer lexer{source};
        auto tokens_result = lexer.lex();
        if (!tokens_result.has_value()) {
            return ChainableResult<std::vector<EnumDeclaration>, CParserError>{tokens_result};
        }
        Parser parser{std::move(tokens_result).value()};
        return parser.parse_enums();
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

TEST_F(ParserTests, DivisionByZeroErrorPosition)
{
    auto result = parse("#define DIV 10 / 0");
    ASSERT_FALSE(result.has_value());
    // The error is chained - check the innermost error (division by zero operator)
    ASSERT_FALSE(result.chain().empty());
    // The "/" operator is at column 16 on line 1
    // We get the outermost error via error(), innermost via chain()[0]
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

TEST_F(ParserTests, UnknownIdentifierErrorPosition)
{
    auto result = parse("#define FOO UNKNOWN_MACRO");
    ASSERT_FALSE(result.has_value());
    // Chain structure:
    // chain[0] = empty wrapper from parse_defines() passthrough
    // chain[1] = "failed to evaluate expression for '#define FOO'" from parse_define()
    // chain[2] = "unknown identifier 'UNKNOWN_MACRO'" from evaluate_postfix()
    ASSERT_GE(result.chain().size(), 3);
    const auto *inner_err = dynamic_cast<const CParserError *>(result.chain()[2].get());
    ASSERT_NE(inner_err, nullptr);
    EXPECT_EQ(inner_err->position().line, 1);
    EXPECT_EQ(inner_err->position().column, 13); // "UNKNOWN_MACRO" starts at column 13
    EXPECT_EQ(inner_err->message(), "unknown identifier 'UNKNOWN_MACRO'");
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

// ============================================================================
// Enum Parsing Tests
// ============================================================================

TEST_F(ParserTests, ParseEmptyEnumReturnsEmptyMembers)
{
    auto result = parse_enums("enum { };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_TRUE(result.value()[0].members().empty());
    EXPECT_FALSE(result.value()[0].has_name());
}

TEST_F(ParserTests, ParseAnonymousEnumWithSimpleMembers)
{
    auto result = parse_enums("enum { FOO, BAR, BAZ };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);

    const auto &members = result.value()[0].members();
    ASSERT_EQ(members.size(), 3);
    EXPECT_EQ(members[0].name(), "FOO");
    EXPECT_EQ(members[0].value(), 0);
    EXPECT_FALSE(members[0].has_explicit_value());
    EXPECT_EQ(members[1].name(), "BAR");
    EXPECT_EQ(members[1].value(), 1);
    EXPECT_EQ(members[2].name(), "BAZ");
    EXPECT_EQ(members[2].value(), 2);
}

TEST_F(ParserTests, ParseNamedEnum)
{
    auto result = parse_enums("enum MyEnum { A, B };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_TRUE(result.value()[0].has_name());
    EXPECT_EQ(result.value()[0].name().value(), "MyEnum");
}

TEST_F(ParserTests, ParseEnumWithExplicitValues)
{
    auto result = parse_enums("enum { A = 10, B, C = 20, D };");
    ASSERT_TRUE(result.has_value());

    const auto &members = result.value()[0].members();
    ASSERT_EQ(members.size(), 4);
    EXPECT_EQ(members[0].name(), "A");
    EXPECT_EQ(members[0].value(), 10);
    EXPECT_TRUE(members[0].has_explicit_value());
    EXPECT_EQ(members[1].name(), "B");
    EXPECT_EQ(members[1].value(), 11); // A + 1
    EXPECT_FALSE(members[1].has_explicit_value());
    EXPECT_EQ(members[2].name(), "C");
    EXPECT_EQ(members[2].value(), 20);
    EXPECT_TRUE(members[2].has_explicit_value());
    EXPECT_EQ(members[3].name(), "D");
    EXPECT_EQ(members[3].value(), 21); // C + 1
}

TEST_F(ParserTests, ParseEnumWithHexValues)
{
    auto result = parse_enums("enum { A = 0x00, B = 0xFF };");
    ASSERT_TRUE(result.has_value());

    const auto &members = result.value()[0].members();
    EXPECT_EQ(members[0].value(), 0);
    EXPECT_EQ(members[1].value(), 255);
}

TEST_F(ParserTests, ParseEnumWithExpressionValues)
{
    auto result = parse_enums("enum { A = 1 << 4, B = (0xFF & 0x0F) };");
    ASSERT_TRUE(result.has_value());

    const auto &members = result.value()[0].members();
    EXPECT_EQ(members[0].value(), 16); // 1 << 4
    EXPECT_EQ(members[1].value(), 15); // 0xFF & 0x0F
}

TEST_F(ParserTests, ParseEnumWithNewlinesAndComments)
{
    auto result = parse_enums(R"(
enum {
    MB_NORMAL,
    MB_TALL_GRASS, // this is a comment
    MB_DEEP_WATER,
};
)");
    ASSERT_TRUE(result.has_value());

    const auto &members = result.value()[0].members();
    ASSERT_EQ(members.size(), 3);
    EXPECT_EQ(members[0].name(), "MB_NORMAL");
    EXPECT_EQ(members[1].name(), "MB_TALL_GRASS");
    EXPECT_EQ(members[2].name(), "MB_DEEP_WATER");
}

TEST_F(ParserTests, ParseMultipleEnums)
{
    auto result = parse_enums("enum A { X }; enum B { Y };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2);
    EXPECT_EQ(result.value()[0].name().value(), "A");
    EXPECT_EQ(result.value()[1].name().value(), "B");
}

TEST_F(ParserTests, ParseEnumMemberWithoutTrailingComma)
{
    auto result = parse_enums("enum { A, B, C };"); // No trailing comma after C
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value()[0].members().size(), 3);
}

TEST_F(ParserTests, ParseEnumWithTrailingComma)
{
    auto result = parse_enums("enum { A, B, C, };"); // Trailing comma after C
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value()[0].members().size(), 3);
}

TEST_F(ParserTests, ParseRealWorldBehaviorEnum)
{
    auto result = parse_enums(R"(
enum {
    MB_NORMAL,
    MB_SECRET_BASE_WALL,
    MB_TALL_GRASS,
    MB_INVALID = 0xFF,
};
)");
    ASSERT_TRUE(result.has_value());

    const auto &members = result.value()[0].members();
    ASSERT_EQ(members.size(), 4);
    EXPECT_EQ(members[0].value(), 0);
    EXPECT_EQ(members[1].value(), 1);
    EXPECT_EQ(members[2].value(), 2);
    EXPECT_EQ(members[3].name(), "MB_INVALID");
    EXPECT_EQ(members[3].value(), 255);
    EXPECT_TRUE(members[3].has_explicit_value());
}

TEST_F(ParserTests, ParseNoEnumsReturnsEmpty)
{
    auto result = parse_enums("#define FOO 123\nint x;");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(ParserTests, EnumMissingOpeningBraceReturnsError)
{
    auto result = parse_enums("enum FOO;");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ParserTests, EnumMissingOpeningBraceErrorPosition)
{
    auto result = parse_enums("enum FOO;");
    ASSERT_FALSE(result.has_value());
    // Chain structure:
    // chain[0] = empty wrapper from parse_enums() passthrough
    // chain[1] = "expected '{' after 'enum'" from parse_enum()
    ASSERT_GE(result.chain().size(), 2);
    const auto *err = dynamic_cast<const CParserError *>(result.chain()[1].get());
    ASSERT_NE(err, nullptr);
    // Error should point to the semicolon position
    EXPECT_EQ(err->position().line, 1);
    EXPECT_EQ(err->position().column, 9);
    EXPECT_EQ(err->message(), "expected '{' after 'enum'");
}

TEST_F(ParserTests, EnumMissingClosingBraceReturnsError)
{
    auto result = parse_enums("enum { A, B");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ParserTests, EnumMissingClosingBraceErrorPosition)
{
    auto result = parse_enums("enum { A, B");
    ASSERT_FALSE(result.has_value());
    // Chain structure:
    // chain[0] = empty wrapper from parse_enums() passthrough
    // chain[1] = "expected '}' to close enum" from parse_enum()
    ASSERT_GE(result.chain().size(), 2);
    const auto *err = dynamic_cast<const CParserError *>(result.chain()[1].get());
    ASSERT_NE(err, nullptr);
    // Error should point to EOF
    EXPECT_EQ(err->position().line, 1);
    EXPECT_EQ(err->message(), "expected '}' to close enum");
}

} // namespace
} // namespace porytiles2
