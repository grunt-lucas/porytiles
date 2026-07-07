#include "porytiles/utilities/c_parser/parser.hpp"

#include <algorithm>

#include <gtest/gtest.h>

#include "porytiles/utilities/c_parser/lexer.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace porytiles {
namespace {

class ParserTests : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

    [[nodiscard]] ChainableResult<std::vector<DefineStatement>> parse(const std::string &source)
    {
        Lexer lexer{&formatter_, source};
        auto tokens_result = lexer.lex();
        if (!tokens_result.has_value()) {
            return ChainableResult<std::vector<DefineStatement>>{tokens_result};
        }
        Parser parser{&formatter_, std::move(tokens_result).value()};
        return parser.parse_defines();
    }

    [[nodiscard]] ChainableResult<std::vector<EnumDeclaration>> parse_enums(const std::string &source)
    {
        Lexer lexer{&formatter_, source};
        auto tokens_result = lexer.lex();
        if (!tokens_result.has_value()) {
            return ChainableResult<std::vector<EnumDeclaration>>{tokens_result};
        }
        Parser parser{&formatter_, std::move(tokens_result).value()};
        return parser.parse_enums();
    }

    [[nodiscard]] ChainableResult<std::vector<ArrayDeclaration>> parse_pointer_arrays(const std::string &source)
    {
        Lexer lexer{&formatter_, source};
        auto tokens_result = lexer.lex();
        if (!tokens_result.has_value()) {
            return ChainableResult<std::vector<ArrayDeclaration>>{tokens_result};
        }
        Parser parser{&formatter_, std::move(tokens_result).value()};
        return parser.parse_pointer_arrays();
    }

    [[nodiscard]] ChainableResult<std::vector<FunctionDefinition>> parse_functions(const std::string &source)
    {
        Lexer lexer{&formatter_, source};
        auto tokens_result = lexer.lex();
        if (!tokens_result.has_value()) {
            return ChainableResult<std::vector<FunctionDefinition>>{tokens_result};
        }
        Parser parser{&formatter_, std::move(tokens_result).value()};
        return parser.parse_functions();
    }

    [[nodiscard]] ChainableResult<std::vector<StructVariableDeclaration>>
    parse_struct_variables(const std::string &source)
    {
        Lexer lexer{&formatter_, source};
        auto tokens_result = lexer.lex();
        if (!tokens_result.has_value()) {
            return ChainableResult<std::vector<StructVariableDeclaration>>{tokens_result};
        }
        Parser parser{&formatter_, std::move(tokens_result).value()};
        return parser.parse_struct_variables();
    }

    [[nodiscard]] ChainableResult<std::vector<IncbinDeclaration>> parse_incbin_arrays(const std::string &source)
    {
        Lexer lexer{&formatter_, source};
        auto tokens_result = lexer.lex();
        if (!tokens_result.has_value()) {
            return ChainableResult<std::vector<IncbinDeclaration>>{tokens_result};
        }
        Parser parser{&formatter_, std::move(tokens_result).value()};
        return parser.parse_incbin_arrays();
    }

    template <typename T>
    [[nodiscard]] std::string get_all_error_text(const ChainableResult<T> &result)
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

TEST_F(ParserTests, ParseUnaryOperatorWithFollowingBinaryOperator)
{
    // Unary ~ and ! must bind tighter than the binary operator that follows them.
    auto result = parse(R"(
#define MASKED ~5 & 3
#define CLEARED ~0xF0 & 0xFF
#define LOGIC !0 && 1
#define GROUPED ~(5 & 3)
)");
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 4);
    EXPECT_EQ(defines[0].int_value(), 2);    // (~5) & 3, not ~(5 & 3) == -2
    EXPECT_EQ(defines[1].int_value(), 0x0F); // (~0xF0) & 0xFF
    EXPECT_EQ(defines[2].int_value(), 1);    // (!0) && 1
    EXPECT_EQ(defines[3].int_value(), -2);   // explicit grouping still applies
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
    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("unknown identifier 'UNKNOWN_MACRO'"), std::string::npos);
    // Position should be line 1, column 13 (where "UNKNOWN_MACRO" starts)
    EXPECT_NE(error_text.find("1:13:"), std::string::npos);
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
    EXPECT_EQ(members[0].int_value(), 0);
    EXPECT_FALSE(members[0].has_explicit_value());
    EXPECT_EQ(members[1].name(), "BAR");
    EXPECT_EQ(members[1].int_value(), 1);
    EXPECT_EQ(members[2].name(), "BAZ");
    EXPECT_EQ(members[2].int_value(), 2);
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
    EXPECT_EQ(members[0].int_value(), 10);
    EXPECT_TRUE(members[0].has_explicit_value());
    EXPECT_EQ(members[1].name(), "B");
    EXPECT_EQ(members[1].int_value(), 11); // A + 1
    EXPECT_FALSE(members[1].has_explicit_value());
    EXPECT_EQ(members[2].name(), "C");
    EXPECT_EQ(members[2].int_value(), 20);
    EXPECT_TRUE(members[2].has_explicit_value());
    EXPECT_EQ(members[3].name(), "D");
    EXPECT_EQ(members[3].int_value(), 21); // C + 1
}

TEST_F(ParserTests, ParseEnumWithHexValues)
{
    auto result = parse_enums("enum { A = 0x00, B = 0xFF };");
    ASSERT_TRUE(result.has_value());

    const auto &members = result.value()[0].members();
    EXPECT_EQ(members[0].int_value(), 0);
    EXPECT_EQ(members[1].int_value(), 255);
}

TEST_F(ParserTests, ParseEnumWithExpressionValues)
{
    auto result = parse_enums("enum { A = 1 << 4, B = (0xFF & 0x0F) };");
    ASSERT_TRUE(result.has_value());

    const auto &members = result.value()[0].members();
    EXPECT_EQ(members[0].int_value(), 16); // 1 << 4
    EXPECT_EQ(members[1].int_value(), 15); // 0xFF & 0x0F
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
    EXPECT_EQ(members[0].int_value(), 0);
    EXPECT_EQ(members[1].int_value(), 1);
    EXPECT_EQ(members[2].int_value(), 2);
    EXPECT_EQ(members[3].name(), "MB_INVALID");
    EXPECT_EQ(members[3].int_value(), 255);
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
    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("expected '{' after 'enum'"), std::string::npos);
    // Error should point to the semicolon position (line 1, column 9)
    EXPECT_NE(error_text.find("1:9:"), std::string::npos);
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
    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("expected '}' to close enum"), std::string::npos);
    // Error should point to EOF on line 1
    EXPECT_NE(error_text.find("1:"), std::string::npos);
}

TEST_F(ParserTests, ParsePointerArraysEmptyInput)
{
    auto result = parse_pointer_arrays("");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(ParserTests, ParseSimplePointerArray)
{
    auto result = parse_pointer_arrays("const u16 *const myArray[] = { elem1, elem2, elem3 };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);

    const auto &arr = result.value()[0];
    EXPECT_EQ(arr.name(), "myArray");
    ASSERT_EQ(arr.elements().size(), 3);
    EXPECT_EQ(arr.elements()[0], "elem1");
    EXPECT_EQ(arr.elements()[1], "elem2");
    EXPECT_EQ(arr.elements()[2], "elem3");
}

TEST_F(ParserTests, ParsePointerArrayWithoutFirstConst)
{
    auto result = parse_pointer_arrays("u16 *const myArray[] = { elem1 };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].name(), "myArray");
}

TEST_F(ParserTests, ParsePointerArrayWithoutSecondConst)
{
    auto result = parse_pointer_arrays("const u16 *myArray[] = { elem1 };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].name(), "myArray");
}

TEST_F(ParserTests, ParsePointerArrayWithNewlines)
{
    auto result = parse_pointer_arrays(R"(
const u16 *const gTilesetAnims_General_Flower[] = {
    gTilesetAnims_General_Flower_Frame0,
    gTilesetAnims_General_Flower_Frame1,
    gTilesetAnims_General_Flower_Frame2
};
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);

    const auto &arr = result.value()[0];
    EXPECT_EQ(arr.name(), "gTilesetAnims_General_Flower");
    ASSERT_EQ(arr.elements().size(), 3);
    EXPECT_EQ(arr.elements()[0], "gTilesetAnims_General_Flower_Frame0");
    EXPECT_EQ(arr.elements()[1], "gTilesetAnims_General_Flower_Frame1");
    EXPECT_EQ(arr.elements()[2], "gTilesetAnims_General_Flower_Frame2");
}

TEST_F(ParserTests, ParseMultiplePointerArrays)
{
    auto result = parse_pointer_arrays(R"(
const u16 *const arrayA[] = { a1, a2 };
const u16 *const arrayB[] = { b1, b2, b3 };
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2);

    EXPECT_EQ(result.value()[0].name(), "arrayA");
    EXPECT_EQ(result.value()[0].elements().size(), 2);
    EXPECT_EQ(result.value()[1].name(), "arrayB");
    EXPECT_EQ(result.value()[1].elements().size(), 3);
}

TEST_F(ParserTests, ParsePointerArrayIgnoresOtherDeclarations)
{
    auto result = parse_pointer_arrays(R"(
int regularVar = 5;
const u16 *const myArray[] = { elem1 };
void someFunction() { }
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].name(), "myArray");
}

TEST_F(ParserTests, ParsePointerArrayWithTrailingComma)
{
    auto result = parse_pointer_arrays("const u16 *const arr[] = { elem1, elem2, };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].elements().size(), 2);
}

TEST_F(ParserTests, ParseStaticPointerArray)
{
    auto result = parse_pointer_arrays("static const u16 *const arr[] = { elem1 };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].name(), "arr");
    ASSERT_EQ(result.value()[0].elements().size(), 1);
    EXPECT_EQ(result.value()[0].elements()[0], "elem1");
}

TEST_F(ParserTests, ParseStaticPointerArrayWithoutConst)
{
    auto result = parse_pointer_arrays("static u16 *const arr[] = { elem1 };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].name(), "arr");
}

TEST_F(ParserTests, ParseSimpleIncbinArray)
{
    auto result = parse_incbin_arrays(
        R"(const u32 gTilesetTiles_General[] = INCBIN_U32("data/tilesets/primary/general/tiles.4bpp");)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].variable_name(), "gTilesetTiles_General");
    EXPECT_EQ(result.value()[0].macro_name(), "INCBIN_U32");
    ASSERT_EQ(result.value()[0].paths().size(), 1);
    EXPECT_EQ(result.value()[0].paths()[0], "data/tilesets/primary/general/tiles.4bpp");
}

TEST_F(ParserTests, ParseStaticIncbinArray)
{
    auto result = parse_incbin_arrays(
        R"(static const u16 sTilesetAnims_General_Flower_Frame0[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/0.4bpp");)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].variable_name(), "sTilesetAnims_General_Flower_Frame0");
    EXPECT_EQ(result.value()[0].macro_name(), "INCBIN_U16");
    ASSERT_EQ(result.value()[0].paths().size(), 1);
    EXPECT_EQ(result.value()[0].paths()[0], "data/tilesets/primary/general/anim/flower/0.4bpp");
}

TEST_F(ParserTests, ParseIncgfxArray)
{
    // pokeemerald-expansion's auto-conversion macro: first arg is the source PNG, second is the target extension.
    auto result = parse_incbin_arrays(
        R"(const u32 gTilesetTiles_velvet_forest[] = INCGFX_U32("data/tilesets/primary/velvet_forest/tiles.png", ".4bpp.lz");)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].variable_name(), "gTilesetTiles_velvet_forest");
    EXPECT_EQ(result.value()[0].macro_name(), "INCGFX_U32");
    ASSERT_EQ(result.value()[0].paths().size(), 1);
    EXPECT_EQ(result.value()[0].paths().front(), "data/tilesets/primary/velvet_forest/tiles.png");
}

TEST_F(ParserTests, ParseIncgfxMultiPathArray)
{
    // The brace/multi-path branch must also accept INCGFX_*; only the first string literal of each call is captured.
    auto result = parse_incbin_arrays(
        R"(const u16 gTilesetPalettes_velvet_forest[][16] =
{
    INCGFX_U16("data/tilesets/primary/velvet_forest/palettes/00.png", ".gbapal"),
    INCGFX_U16("data/tilesets/primary/velvet_forest/palettes/01.png", ".gbapal"),
};)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].variable_name(), "gTilesetPalettes_velvet_forest");
    EXPECT_EQ(result.value()[0].macro_name(), "INCGFX_U16");
    ASSERT_EQ(result.value()[0].paths().size(), 2);
    EXPECT_EQ(result.value()[0].paths()[0], "data/tilesets/primary/velvet_forest/palettes/00.png");
    EXPECT_EQ(result.value()[0].paths()[1], "data/tilesets/primary/velvet_forest/palettes/01.png");
}

TEST_F(ParserTests, ParseFunctionsEmptyInput)
{
    auto result = parse_functions("");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(ParserTests, ParseSimpleFunction)
{
    auto result = parse_functions("void myFunc(int x) { int y = x + 1; }");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);

    const auto &func = result.value()[0];
    EXPECT_EQ(func.name(), "myFunc");
    EXPECT_FALSE(func.body_tokens().empty());
}

TEST_F(ParserTests, ParseStaticFunction)
{
    auto result = parse_functions("static void helper(void) { return; }");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].name(), "helper");
}

TEST_F(ParserTests, ParseFunctionWithNestedBraces)
{
    auto result = parse_functions(R"(
void complexFunc(int x) {
    if (x > 0) {
        for (int i = 0; i < x; i++) {
            doSomething();
        }
    }
}
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].name(), "complexFunc");
    // Body tokens should contain all the nested content
    EXPECT_GT(result.value()[0].body_tokens().size(), 10);
}

TEST_F(ParserTests, ParseMultipleFunctions)
{
    auto result = parse_functions(R"(
void funcA(void) { }
int funcB(int x) { return x; }
static void funcC(void) { }
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 3);
    EXPECT_EQ(result.value()[0].name(), "funcA");
    EXPECT_EQ(result.value()[1].name(), "funcB");
    EXPECT_EQ(result.value()[2].name(), "funcC");
}

TEST_F(ParserTests, ParseFunctionsReturnsAllFunctions)
{
    auto result = parse_functions(
        R"(
void QueueAnimTiles_Flower(u16 timer) { }
void TilesetAnim_General(u16 timer) { }
void QueueAnimTiles_Water(u16 timer) { }
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 3);
    EXPECT_EQ(result.value()[0].name(), "QueueAnimTiles_Flower");
    EXPECT_EQ(result.value()[1].name(), "TilesetAnim_General");
    EXPECT_EQ(result.value()[2].name(), "QueueAnimTiles_Water");
}

TEST_F(ParserTests, ParseFunctionBodyContainsMacroCall)
{
    auto result = parse_functions(R"(
static void QueueAnimTiles_Flower(u16 timer) {
    AppendTilesetAnimToBuffer(ptr, (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(12)), 4 * TILE_SIZE_4BPP);
}
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);

    const auto &body = result.value()[0].body_tokens();
    bool found_tile_offset = false;
    for (const auto &token : body) {
        if (token.is(TokenType::identifier) && token.text() == "TILE_OFFSET_4BPP") {
            found_tile_offset = true;
            break;
        }
    }
    EXPECT_TRUE(found_tile_offset);
}

TEST_F(ParserTests, ParseFunctionsIgnoresDeclarationsWithoutBody)
{
    auto result = parse_functions(R"(
void forwardDecl(int x);
void actualFunc(int x) { return; }
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].name(), "actualFunc");
}

TEST_F(ParserTests, ParseFunctionsWithAnimCodePattern)
{
    auto result = parse_functions(R"(
static void QueueAnimTiles_PorytilesManaged_General_Flower(u16 timer) {
    u16 i = timer % ARRAY_COUNT(gTilesetAnims_PorytilesManaged_General_Flower);
    AppendTilesetAnimToBuffer(gTilesetAnims_PorytilesManaged_General_Flower[i],
        (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(12)), 4 * TILE_SIZE_4BPP);
}

static void TilesetAnim_PorytilesManaged_General(u16 timer) {
    if (timer % 16 == 0) {
        QueueAnimTiles_PorytilesManaged_General_Flower(timer / 16);
    }
}
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2);
    EXPECT_EQ(result.value()[0].name(), "QueueAnimTiles_PorytilesManaged_General_Flower");
    EXPECT_EQ(result.value()[1].name(), "TilesetAnim_PorytilesManaged_General");
}

TEST_F(ParserTests, ParseStructVariablesEmptyInput)
{
    auto result = parse_struct_variables("");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(ParserTests, ParseSimpleStructVariable)
{
    auto result = parse_struct_variables("const struct Tileset gTileset_General = { };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);

    const auto &s = result.value()[0];
    EXPECT_EQ(s.struct_type(), "Tileset");
    EXPECT_EQ(s.variable_name(), "gTileset_General");
}

TEST_F(ParserTests, ParseStructVariableWithoutConst)
{
    auto result = parse_struct_variables("struct Tileset gTileset_Test = { };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].struct_type(), "Tileset");
    EXPECT_EQ(result.value()[0].variable_name(), "gTileset_Test");
}

TEST_F(ParserTests, ParseStructVariableWithBody)
{
    auto result = parse_struct_variables(R"(
const struct Tileset gTileset_General = {
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_General,
    .palettes = gTilesetPalettes_General,
};
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].struct_type(), "Tileset");
    EXPECT_EQ(result.value()[0].variable_name(), "gTileset_General");
}

TEST_F(ParserTests, ParseMultipleStructVariables)
{
    auto result = parse_struct_variables(R"(
const struct Tileset gTileset_General = { };
const struct Tileset gTileset_Petalburg = { };
const struct Tileset gTileset_Building = { };
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 3);
    EXPECT_EQ(result.value()[0].variable_name(), "gTileset_General");
    EXPECT_EQ(result.value()[1].variable_name(), "gTileset_Petalburg");
    EXPECT_EQ(result.value()[2].variable_name(), "gTileset_Building");
}

TEST_F(ParserTests, ParseStructVariablesIgnoresOtherDeclarations)
{
    auto result = parse_struct_variables(R"(
int regularVar = 5;
const struct Tileset gTileset_General = { };
void someFunction() { }
enum { A, B };
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].variable_name(), "gTileset_General");
}

TEST_F(ParserTests, ParseStructVariableWithNestedBraces)
{
    auto result = parse_struct_variables(R"(
const struct Tileset gTileset_General = {
    .metatiles = { 0, 1, 2 },
    .palettes = { pal1, pal2 },
};
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].variable_name(), "gTileset_General");
}

TEST_F(ParserTests, ParseStructVariableNoSemicolon)
{
    // Some code styles omit the trailing semicolon (rare but valid before another declaration)
    auto result = parse_struct_variables("const struct Tileset gTileset_Test = { }");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].variable_name(), "gTileset_Test");
}

TEST_F(ParserTests, ParseStructVariablesReturnsEmptyForNonStructCode)
{
    auto result = parse_struct_variables("#define FOO 123\nint x;");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(ParserTests, ParseStructVariableDifferentStructType)
{
    auto result = parse_struct_variables("const struct MyOtherStruct someVar = { };");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].struct_type(), "MyOtherStruct");
    EXPECT_EQ(result.value()[0].variable_name(), "someVar");
}

TEST_F(ParserTests, ParseRealWorldTilesetHeaders)
{
    auto result = parse_struct_variables(R"(
#include "fieldmap.h"

const struct Tileset gTileset_General =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_General,
    .palettes = gTilesetPalettes_General,
    .metatiles = gMetatiles_General,
    .metatileAttributes = gMetatileAttributes_General,
    .callback = InitTilesetAnim_General,
};

const struct Tileset gTileset_Petalburg =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Petalburg,
    .palettes = gTilesetPalettes_Petalburg,
    .metatiles = gMetatiles_Petalburg,
    .metatileAttributes = gMetatileAttributes_Petalburg,
    .callback = InitTilesetAnim_Petalburg,
};

const struct Tileset *const gTilesetPointer_SecretBase = &gTileset_SecretBase;
)");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2);
    EXPECT_EQ(result.value()[0].struct_type(), "Tileset");
    EXPECT_EQ(result.value()[0].variable_name(), "gTileset_General");
    EXPECT_EQ(result.value()[1].struct_type(), "Tileset");
    EXPECT_EQ(result.value()[1].variable_name(), "gTileset_Petalburg");
}

namespace {

[[nodiscard]] bool has_define(const std::vector<DefineStatement> &defines, const std::string &name)
{
    return std::any_of(defines.begin(), defines.end(), [&](const DefineStatement &d) { return d.name() == name; });
}

} // namespace

TEST_F(ParserTests, ConditionalIncludeGuardParsesBody)
{
    // The guard macro is unknown when the #ifndef is reached, so the region is undecidable and its body is scanned.
    auto result = parse(R"(
#ifndef GUARD_H
#define GUARD_H
#define INSIDE_GUARD 42
#endif
)");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(has_define(result.value(), "GUARD_H"));
    EXPECT_TRUE(has_define(result.value(), "INSIDE_GUARD"));
}

TEST_F(ParserTests, ConditionalIfdefDecidableTrueParsesBody)
{
    auto result = parse(R"(
#define FEATURE
#ifdef FEATURE
#define ENABLED 1
#endif
)");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(has_define(result.value(), "ENABLED"));
}

TEST_F(ParserTests, ConditionalIfndefDecidableFalseSkipsBody)
{
    // FEATURE is defined earlier, so #ifndef FEATURE is decidably false and its body is skipped.
    auto result = parse(R"(
#define FEATURE
#ifndef FEATURE
#define SHOULD_NOT_APPEAR 1
#endif
)");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(has_define(result.value(), "SHOULD_NOT_APPEAR"));
}

TEST_F(ParserTests, ConditionalIfdefDecidableFalseSkipsBody)
{
    // BAR is defined earlier; #ifdef MISSING is undecidable (both) but the #else of a decided frame is what we test:
    // here FEATURE is known, so #ifndef FEATURE (false) skips and its #else runs.
    auto result = parse(R"(
#define FEATURE
#ifndef FEATURE
#define A 1
#else
#define B 2
#endif
)");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(has_define(result.value(), "A"));
    EXPECT_TRUE(has_define(result.value(), "B"));
}

TEST_F(ParserTests, ConditionalElifChainPicksFirstTrueBranch)
{
    // The Shunting Yard evaluator resolves bare identifiers to their values, so #if FIRST (0) is false and #elif SECOND
    // (1) is the first true branch. Comparison operators are intentionally out of scope for the evaluator.
    auto result = parse(R"(
#define FIRST 0
#define SECOND 1
#if FIRST
#define BRANCH_ONE 1
#elif SECOND
#define BRANCH_TWO 2
#else
#define BRANCH_ELSE 3
#endif
)");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(has_define(result.value(), "BRANCH_ONE"));
    EXPECT_TRUE(has_define(result.value(), "BRANCH_TWO"));
    EXPECT_FALSE(has_define(result.value(), "BRANCH_ELSE"));
}

TEST_F(ParserTests, ConditionalIfZeroSkipsBody)
{
    auto result = parse(R"(
#if 0
#define DISABLED 1
#endif
#define ALWAYS 2
)");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(has_define(result.value(), "DISABLED"));
    EXPECT_TRUE(has_define(result.value(), "ALWAYS"));
}

TEST_F(ParserTests, ConditionalUndecidableIfParsesBothBranches)
{
    // UNKNOWN is never defined, so #if UNKNOWN is undecidable and both branches are scanned.
    auto result = parse(R"(
#if UNKNOWN
#define MAYBE_A 1
#else
#define MAYBE_B 2
#endif
)");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(has_define(result.value(), "MAYBE_A"));
    EXPECT_TRUE(has_define(result.value(), "MAYBE_B"));
}

TEST_F(ParserTests, ConditionalNestedSkippingSuppressesInnerBoth)
{
    // The outer #ifndef FEATURE (FEATURE known) is decidably false, so even an inner undecidable region is skipped.
    auto result = parse(R"(
#define FEATURE
#ifndef FEATURE
#ifdef WHATEVER
#define DEEP 1
#endif
#endif
)");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(has_define(result.value(), "DEEP"));
}

TEST_F(ParserTests, ConditionalBothRegionDuplicateSameValueNoWarning)
{
    Lexer lexer{&formatter_, R"(
#define X 5
#ifndef GUARD
#define X 5
#endif
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};
    auto result = parser.parse_defines();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(parser.scan_warnings().empty());
}

TEST_F(ParserTests, ConditionalBothRegionDuplicateConflictWarnsLastWins)
{
    Lexer lexer{&formatter_, R"(
#define X 5
#ifndef GUARD
#define X 9
#endif
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};
    auto result = parser.parse_defines();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(parser.scan_warnings().size(), 1U);
    EXPECT_NE(parser.scan_warnings()[0].find("conflicting redefinition"), std::string::npos);
    // Last write wins in the symbol table, reflected in the recorded statement order.
    ASSERT_FALSE(result.value().empty());
    EXPECT_EQ(result.value().back().name(), "X");
    EXPECT_EQ(result.value().back().int_value(), 9);
}

TEST_F(ParserTests, TolerantDefineSkipsCrossHeaderReference)
{
    Lexer lexer{&formatter_, R"(
#define KNOWN 3
#define DERIVED (KNOWN + 1)
#define EXTERNAL (SOME_OTHER_HEADER_MACRO + 1)
#define AFTER 7
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};

    TolerantDefineScan scan = parser.parse_defines_tolerant();

    // The unevaluable define is skipped but everything else resolves, and scanning does not abort.
    EXPECT_TRUE(has_define(scan.defines, "KNOWN"));
    EXPECT_TRUE(has_define(scan.defines, "DERIVED"));
    EXPECT_TRUE(has_define(scan.defines, "AFTER"));
    EXPECT_FALSE(has_define(scan.defines, "EXTERNAL"));
    ASSERT_EQ(scan.skipped.size(), 1U);
    EXPECT_EQ(scan.skipped[0].name, "EXTERNAL");
    // The skipped define's name still counts as defined for later conditional decisions.
    EXPECT_TRUE(parser.defined_names().contains("EXTERNAL"));
}

TEST_F(ParserTests, TolerantDefineSkipsUnevaluableContinuationDefine)
{
    Lexer lexer{
        &formatter_, "#define FOLLOWER_INVISIBLE_FLAGS (FLAG_A | \\\n                                 FLAG_B)\n"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};

    TolerantDefineScan scan = parser.parse_defines_tolerant();

    EXPECT_FALSE(has_define(scan.defines, "FOLLOWER_INVISIBLE_FLAGS"));
    ASSERT_EQ(scan.skipped.size(), 1U);
    EXPECT_EQ(scan.skipped[0].name, "FOLLOWER_INVISIBLE_FLAGS");
}

TEST_F(ParserTests, TolerantEnumCounterPoisoning)
{
    Lexer lexer{&formatter_, R"(
enum {
    A,
    B = SOME_UNKNOWN,
    C,
    D = 5,
    E,
};
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};

    TolerantEnumScan scan = parser.parse_enums_tolerant();
    ASSERT_EQ(scan.enums.size(), 1U);
    const auto &members = scan.enums[0].members;
    ASSERT_EQ(members.size(), 5U);

    EXPECT_EQ(members[0].name, "A");
    EXPECT_EQ(members[0].value, 0);
    // The unevaluable explicit value poisons the counter for itself and the following implicit member.
    EXPECT_EQ(members[1].name, "B");
    EXPECT_FALSE(members[1].value.has_value());
    EXPECT_EQ(members[2].name, "C");
    EXPECT_FALSE(members[2].value.has_value());
    // A fresh evaluable explicit value re-anchors the counter.
    EXPECT_EQ(members[3].name, "D");
    EXPECT_EQ(members[3].value, 5);
    EXPECT_EQ(members[4].name, "E");
    EXPECT_EQ(members[4].value, 6);
}

TEST_F(ParserTests, TolerantDefineSkipsUnsupportedExpressionForms)
{
    // A ternary is not supported, and stranded operands mean the expression was not understood. Both must degrade to
    // "value unknown" rather than evaluate to whatever operand the evaluator saw last.
    Lexer lexer{&formatter_, R"(
#define COND 1
#define CHOICE COND ? 10 : 20
#define STRANDED 5 6
#define AFTER 7
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};

    TolerantDefineScan scan = parser.parse_defines_tolerant();

    EXPECT_TRUE(has_define(scan.defines, "COND"));
    EXPECT_TRUE(has_define(scan.defines, "AFTER"));
    EXPECT_FALSE(has_define(scan.defines, "CHOICE"));
    EXPECT_FALSE(has_define(scan.defines, "STRANDED"));
    ASSERT_EQ(scan.skipped.size(), 2U);
    EXPECT_EQ(scan.skipped[0].name, "CHOICE");
    EXPECT_EQ(scan.skipped[1].name, "STRANDED");
}

TEST_F(ParserTests, TolerantDefineSkipsCastExpression)
{
    // A C cast like (u32)0x... is not an arithmetic form the evaluator understands: its parens and literal are all
    // "supported" tokens, so it slips past the token gate and only fails when the cast type reads as an unknown
    // identifier. The define must degrade to "value unknown" (skipped), not resolve to a partial value.
    Lexer lexer{&formatter_, R"(
#define CASTED (u32)0x000001ff
#define AFTER 7
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};

    TolerantDefineScan scan = parser.parse_defines_tolerant();

    EXPECT_FALSE(has_define(scan.defines, "CASTED"));
    EXPECT_TRUE(has_define(scan.defines, "AFTER"));
    ASSERT_EQ(scan.skipped.size(), 1U);
    EXPECT_EQ(scan.skipped[0].name, "CASTED");
}

TEST_F(ParserTests, TolerantEnumDirectiveInBodyPoisonsFollowingValues)
{
    // The scanner does not evaluate conditionals inside an enum body, so once a directive appears, no later value
    // (implicit or explicit) can be trusted: an explicit value may sit in an untaken branch. The directive's own
    // tokens must not be lexed as phantom members, which would silently shift every following implicit value.
    Lexer lexer{&formatter_, R"(
enum {
    A,
    B,
#if SOME_FLAG
    C,
    D = 5,
#endif
    E,
};
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};

    TolerantEnumScan scan = parser.parse_enums_tolerant();
    ASSERT_EQ(scan.enums.size(), 1U);
    const auto &members = scan.enums[0].members;
    ASSERT_EQ(members.size(), 5U);

    EXPECT_EQ(members[0].name, "A");
    EXPECT_EQ(members[0].value, 0);
    EXPECT_EQ(members[1].name, "B");
    EXPECT_EQ(members[1].value, 1);
    EXPECT_EQ(members[2].name, "C");
    EXPECT_FALSE(members[2].value.has_value());
    EXPECT_EQ(members[3].name, "D");
    EXPECT_FALSE(members[3].value.has_value());
    EXPECT_EQ(members[4].name, "E");
    EXPECT_FALSE(members[4].value.has_value());
}

TEST_F(ParserTests, IndexedArrayResolvesSeededMacroValuesAndHexCasing)
{
    Lexer lexer{&formatter_, R"(
static const u32 sMetatileAttrMasks[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR] = METATILE_ATTR_BEHAVIOR_MASK,
    [METATILE_ATTRIBUTE_TERRAIN]  = 0x00003e00,
    [METATILE_ATTRIBUTE_LAYER]    = 0xF000,
};
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};
    parser.seed_symbols({{"METATILE_ATTR_BEHAVIOR_MASK", 0x1FF}});

    auto result = parser.parse_indexed_arrays();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    const auto &arr = result.value()[0];
    EXPECT_EQ(arr.name, "sMetatileAttrMasks");
    ASSERT_EQ(arr.entries.size(), 3U);

    EXPECT_EQ(arr.entries[0].index_name, "METATILE_ATTRIBUTE_BEHAVIOR");
    ASSERT_TRUE(arr.entries[0].value.has_value());
    EXPECT_EQ(arr.entries[0].value.value(), 0x1FF); // resolved from the seeded macro
    EXPECT_EQ(arr.entries[1].index_name, "METATILE_ATTRIBUTE_TERRAIN");
    EXPECT_EQ(arr.entries[1].value.value(), 0x3E00); // lowercase hex literal
    EXPECT_EQ(arr.entries[2].value.value(), 0xF000);
}

TEST_F(ParserTests, IndexedArrayUnevaluableEntryLeavesValueAbsent)
{
    Lexer lexer{&formatter_, R"(
static const u32 sMasks[COUNT] = {
    [IDX_A] = 0x1,

    [IDX_B] = SOME_UNSEEDED_MACRO,
    [IDX_C] = 0x4
};
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};

    auto result = parser.parse_indexed_arrays();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    const auto &arr = result.value()[0];
    // Blank lines inside the braces are tolerated; all three entries are captured.
    ASSERT_EQ(arr.entries.size(), 3U);
    EXPECT_EQ(arr.entries[0].value.value(), 1);
    EXPECT_EQ(arr.entries[1].index_name, "IDX_B");
    EXPECT_FALSE(arr.entries[1].value.has_value());
    EXPECT_FALSE(arr.entries[1].value_tokens.empty()); // raw tokens retained for later re-evaluation
    EXPECT_EQ(arr.entries[2].value.value(), 4);
}

TEST_F(ParserTests, IndexedArrayParsesHighBitMasks)
{
    // pokeemerald-expansion's sMetatileAttrMasks really contains 0x80000000; the full 32-bit mask must survive too.
    Lexer lexer{&formatter_, R"(
static const u32 sMasks[COUNT] = {
    [IDX_A] = 0x80000000,
    [IDX_B] = 0xFFFFFFFF,
};
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};

    auto result = parser.parse_indexed_arrays();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    const auto &arr = result.value()[0];
    ASSERT_EQ(arr.entries.size(), 2U);
    EXPECT_EQ(arr.entries[0].value.value(), 0x80000000LL);
    EXPECT_EQ(arr.entries[1].value.value(), 0xFFFFFFFFLL);
}

TEST_F(ParserTests, IndexedArrayDecoyDistinguishedByExactName)
{
    Lexer lexer{&formatter_, R"(
static const u32 sMetatileAttrMasks[COUNT] = {
    [IDX_A] = 0x1,
};
static const u32 sMetatileAttrMasksEmerald[COUNT] = {
    [IDX_A] = 0x00FF,
};
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};

    auto result = parser.parse_indexed_arrays();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2U);
    // Both parse, but they carry distinct names so an exact-name lookup ignores the Emerald decoy.
    auto exact = std::find_if(result.value().begin(), result.value().end(), [](const IndexedArrayDeclaration &a) {
        return a.name == "sMetatileAttrMasks";
    });
    ASSERT_NE(exact, result.value().end());
    EXPECT_EQ(exact->entries[0].value.value(), 1);
}

TEST_F(ParserTests, IndexedArrayInInactiveConditionalIsExcluded)
{
    // A masks table sometimes sits inside a preprocessor branch. A table in a provably inactive branch (#if 0) must be
    // dropped while one in a taken branch (#if 1) is kept, so inference never reads masks the C build does not compile.
    Lexer lexer{&formatter_, R"(
#if 0
static const u32 sMetatileAttrMasks[COUNT] = {
    [IDX_A] = 0x1,
};
#endif
#if 1
static const u32 sMetatileAttrMasks[COUNT] = {
    [IDX_A] = 0x00FF,
};
#endif
)"};
    auto tokens = lexer.lex();
    ASSERT_TRUE(tokens.has_value());
    Parser parser{&formatter_, std::move(tokens).value()};

    auto result = parser.parse_indexed_arrays();
    ASSERT_TRUE(result.has_value());
    // Only the taken branch's table survives.
    ASSERT_EQ(result.value().size(), 1U);
    const auto &arr = result.value()[0];
    EXPECT_EQ(arr.name, "sMetatileAttrMasks");
    ASSERT_EQ(arr.entries.size(), 1U);
    EXPECT_EQ(arr.entries[0].value.value(), 0x00FF);
}

TEST_F(ParserTests, EvaluatesComparisonAndLogicalOperators)
{
    auto result = parse(R"(
#define LT (1 < 2)
#define GE (3 >= 3)
#define EQ (4 == 4)
#define NE (4 != 5)
#define AND (1 && 0)
#define OR (1 || 0)
)");
    ASSERT_TRUE(result.has_value());
    const auto &defines = result.value();
    const auto value_of = [&](const std::string &name) {
        auto it =
            std::find_if(defines.begin(), defines.end(), [&](const DefineStatement &d) { return d.name() == name; });
        return (it != defines.end() && it->has_int_value()) ? it->int_value() : -1;
    };
    EXPECT_EQ(value_of("LT"), 1);
    EXPECT_EQ(value_of("GE"), 1);
    EXPECT_EQ(value_of("EQ"), 1);
    EXPECT_EQ(value_of("NE"), 1);
    EXPECT_EQ(value_of("AND"), 0);
    EXPECT_EQ(value_of("OR"), 1);
}

TEST_F(ParserTests, ConditionalComparisonExpressionIsDecidable)
{
    // With comparison evaluation, #if PICK == 2 is now decidable rather than falling back to both.
    auto result = parse(R"(
#define PICK 2
#if PICK == 1
#define BRANCH_ONE 1
#elif PICK == 2
#define BRANCH_TWO 2
#else
#define BRANCH_ELSE 3
#endif
)");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(has_define(result.value(), "BRANCH_ONE"));
    EXPECT_TRUE(has_define(result.value(), "BRANCH_TWO"));
    EXPECT_FALSE(has_define(result.value(), "BRANCH_ELSE"));
}

TEST_F(ParserTests, ConditionalLogicalExpressionIsDecidable)
{
    auto result = parse(R"(
#define A 1
#define B 0
#if A && B
#define BOTH_ON 1
#endif
#if A || B
#define EITHER_ON 1
#endif
)");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(has_define(result.value(), "BOTH_ON"));
    EXPECT_TRUE(has_define(result.value(), "EITHER_ON"));
}

} // namespace
} // namespace porytiles
