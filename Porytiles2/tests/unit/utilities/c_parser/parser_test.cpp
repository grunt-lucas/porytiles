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
    // Should contain the error message and position info
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
    // Should contain the error message
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
    // Should contain the error message
    EXPECT_NE(error_text.find("expected '}' to close enum"), std::string::npos);
    // Error should point to EOF on line 1
    EXPECT_NE(error_text.find("1:"), std::string::npos);
}

// ============================================================================
// Pointer Array Parsing Tests
// ============================================================================

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

// ============================================================================
// INCBIN Array Parsing Tests
// ============================================================================

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

// ============================================================================
// Function Definition Parsing Tests
// ============================================================================

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
    // Should contain TILE_OFFSET_4BPP identifier
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

// ============================================================================
// Struct Variable Declaration Parsing Tests
// ============================================================================

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

} // namespace
} // namespace porytiles2
