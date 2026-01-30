#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/utilities/c_parser/array_declaration.hpp"
#include "porytiles2/utilities/c_parser/define_statement.hpp"
#include "porytiles2/utilities/c_parser/enum_declaration.hpp"
#include "porytiles2/utilities/c_parser/function_definition.hpp"
#include "porytiles2/utilities/c_parser/incbin_declaration.hpp"
#include "porytiles2/utilities/c_parser/source_position.hpp"
#include "porytiles2/utilities/c_parser/struct_initializer_declaration.hpp"
#include "porytiles2/utilities/c_parser/struct_variable_declaration.hpp"
#include "porytiles2/utilities/c_parser/token.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

class CParserContext;

/**
 * @brief Parser for C preprocessor constructs.
 *
 * @details
 * Parser analyzes a token stream and extracts structured information. The initial implementation focuses on parsing
 * #define statements, extracting macro names and evaluating constant expressions.
 *
 * The parser uses the Shunting Yard algorithm to evaluate arithmetic expressions in #define values, supporting
 * operators: +, -, *, /, %, &, |, ^, ~, <<, >>
 *
 * Example usage:
 * @code
 * Lexer lexer{source_content};
 * auto tokens_result = lexer.lex();
 * if (tokens_result.has_value()) {
 *     Parser parser{std::move(tokens_result).value()};
 *     auto defines_result = parser.parse_defines();
 *     if (defines_result.has_value()) {
 *         for (const auto& def : defines_result.value()) {
 *             // process defines
 *         }
 *     }
 * }
 * @endcode
 *
 * Future extensions will add methods like:
 * - parse_functions() for function declarations/definitions
 * - parse_structs() for struct definitions
 */
class Parser {
  public:
    /**
     * @brief Constructs a parser for the given token stream.
     *
     * @details
     * This constructor creates a parser without a CParserContext. Errors will be formatted as simple "line:col:
     * message" strings without source context highlighting.
     *
     * @param format The text formatter for styled output
     * @param tokens The tokens to parse (typically from Lexer::lex())
     */
    Parser(gsl::not_null<const TextFormatter *> format, std::vector<Token> tokens)
        : format_{format}, tokens_{std::move(tokens)}
    {
    }

    /**
     * @brief Constructs a parser with a context for rich error formatting.
     *
     * @details
     * This constructor creates a parser with a CParserContext that enables rich error formatting with source context
     * highlighting via FileHighlightPrinter. The context must outlive the parser.
     *
     * @param format The text formatter for styled output
     * @param tokens The tokens to parse (typically from Lexer::lex())
     * @param context The parser context for rich error formatting (non-owning)
     */
    Parser(gsl::not_null<const TextFormatter *> format, std::vector<Token> tokens, const CParserContext *context)
        : format_{format}, tokens_{std::move(tokens)}, context_{context}
    {
    }

    /**
     * @brief Parses all #define statements from the token stream.
     *
     * @details
     * Scans through the token stream looking for #define directives. For each one found, parses the macro name and
     * evaluates the value expression if present. Non-define preprocessor directives and other code constructs are
     * skipped.
     *
     * Supports:
     * - Simple integer defines: #define FOO 123
     * - Hex/octal/binary literals: #define BAR 0xFF
     * - Arithmetic expressions: #define BAZ (1 << 4)
     * - String defines: #define MSG "hello"
     * - Flag defines: #define DEBUG
     * - References to previously defined macros: #define B A (where A was defined earlier)
     *
     * When constructed with a CParserContext, errors include source code context with highlighted error locations. When
     * constructed without a context, errors are simple "line:col: message" format.
     *
     * @return A vector of DefineStatement on success, or an error on failure
     */
    [[nodiscard]] ChainableResult<std::vector<DefineStatement>> parse_defines();

    /**
     * @brief Parses all enum declarations from the token stream.
     *
     * @details
     * Scans through the token stream looking for enum declarations. For each one found, parses the optional enum name
     * and all members with their values. Supports both implicit counter-based values and explicit value assignments.
     *
     * Supports:
     * - Anonymous enums: enum { ... }
     * - Named enums: enum Name { ... }
     * - Simple members: FOO,
     * - Explicit values: FOO = 10,
     * - Expression values: FOO = (1 << 4),
     * - References to previously defined macros: FOO = SOME_DEFINE,
     *
     * When constructed with a CParserContext, errors include source code context with highlighted error locations. When
     * constructed without a context, errors are simple "line:col: message" format.
     *
     * @return A vector of EnumDeclaration on success, or an error on failure
     */
    [[nodiscard]] ChainableResult<std::vector<EnumDeclaration>> parse_enums();

    /**
     * @brief Parses all pointer array declarations from the token stream.
     *
     * @details
     * Scans through the token stream looking for pointer array declarations matching the pattern:
     * @code
     * [const] TYPE * [const] IDENTIFIER [] = { element1, element2, ... };
     * @endcode
     *
     * This is used by AnimCodeParser to extract animation frame arrays like:
     * @code
     * const u16 *const gTilesetAnims_General_Flower[] = {
     *     gTilesetAnims_General_Flower_Frame0,
     *     gTilesetAnims_General_Flower_Frame1
     * };
     * @endcode
     *
     * The parser extracts the array name and all identifier elements from the initializer list.
     *
     * @return A vector of ArrayDeclaration on success, or an error on failure
     */
    [[nodiscard]] ChainableResult<std::vector<ArrayDeclaration>> parse_pointer_arrays();

    /**
     * @brief Parses function definitions from the token stream.
     *
     * @details
     * Scans through the token stream looking for function definitions matching the pattern:
     * @code
     * [static] TYPE IDENTIFIER ( params ) { body }
     * @endcode
     *
     * This is used by AnimCodeParser to extract queue and driver functions like:
     * @code
     * static void QueueAnimTiles_General_Flower(u16 timer) {
     *     AppendTilesetAnimToBuffer(..., TILE_OFFSET_4BPP(12), 4 * TILE_SIZE_4BPP);
     * }
     * @endcode
     *
     * The parser captures the function name and all tokens within the body braces for later pattern matching.
     *
     * @return A vector of FunctionDefinition on success, or an error on failure
     */
    [[nodiscard]] ChainableResult<std::vector<FunctionDefinition>> parse_functions();

    /**
     * @brief Parses struct variable declarations from the token stream.
     *
     * @details
     * Scans through the token stream looking for struct variable declarations matching the pattern:
     * @code
     * [const] struct TYPE IDENTIFIER = { ... } [;]
     * @endcode
     *
     * This is used by ProjectTilesetNameProvider to extract tileset names from headers.h files like:
     * @code
     * const struct Tileset gTileset_General = {
     *     .isCompressed = TRUE,
     *     .isSecondary = FALSE,
     *     ...
     * };
     * @endcode
     *
     * The parser captures the struct type and variable name; the initializer body is skipped.
     *
     * @return A vector of StructVariableDeclaration on success, or an error on failure
     */
    [[nodiscard]] ChainableResult<std::vector<StructVariableDeclaration>> parse_struct_variables();

    /**
     * @brief Parses struct variable declarations with their designated initializer fields.
     *
     * @details
     * Scans through the token stream looking for struct variable declarations matching the pattern:
     * @code
     * [const] struct TYPE IDENTIFIER = { .field1 = value1, .field2 = value2, ... } [;]
     * @endcode
     *
     * This is used by ProjectTilesetMetadataProvider to extract tileset metadata from headers.h files:
     * @code
     * const struct Tileset gTileset_General = {
     *     .isCompressed = TRUE,
     *     .isSecondary = FALSE,
     *     .tiles = gTilesetTiles_General,
     *     .palettes = gTilesetPalettes_General,
     *     .metatiles = gMetatiles_General,
     *     .metatileAttributes = gMetatileAttributes_General,
     *     .callback = InitTilesetAnim_General,
     * };
     * @endcode
     *
     * Unlike parse_struct_variables(), this method also parses the designated initializer fields,
     * enabling extraction of field values like isSecondary and variable references.
     *
     * @return A vector of StructInitializerDeclaration on success, or an error on failure
     */
    [[nodiscard]] ChainableResult<std::vector<StructInitializerDeclaration>> parse_struct_initializers();

    /**
     * @brief Parses INCBIN array declarations from the token stream.
     *
     * @details
     * Scans through the token stream looking for array declarations using INCBIN macros:
     * @code
     * // Single path arrays:
     * [const] TYPE IDENTIFIER [] = INCBIN_MACRO("path");
     *
     * // Multi-path arrays (palettes):
     * [const] TYPE IDENTIFIER [][SIZE] = { INCBIN_MACRO("path1"), INCBIN_MACRO("path2"), ... };
     * @endcode
     *
     * Examples from pokeemerald:
     * @code
     * const u32 gTilesetTiles_General[] = INCBIN_U32("data/tilesets/primary/general/tiles.4bpp");
     *
     * const u16 gTilesetPalettes_General[][16] = {
     *     INCBIN_U16("data/tilesets/primary/general/palettes/00.gbapal"),
     *     INCBIN_U16("data/tilesets/primary/general/palettes/01.gbapal"),
     *     ...
     * };
     * @endcode
     *
     * The parser extracts the variable name, INCBIN macro name, and path(s) from each declaration.
     *
     * @return A vector of IncbinDeclaration on success, or an error on failure
     */
    [[nodiscard]] ChainableResult<std::vector<IncbinDeclaration>> parse_incbin_arrays();

  private:
    [[nodiscard]] const Token &peek() const;
    [[nodiscard]] const Token &peek_next() const;
    const Token &advance();
    [[nodiscard]] bool is_at_end() const;
    [[nodiscard]] bool check(TokenType type) const;
    bool match(TokenType type);

    void skip_to_next_line();
    [[nodiscard]] bool is_at_line_end() const;
    [[nodiscard]] ChainableResult<DefineStatement> parse_define();
    [[nodiscard]] ChainableResult<EnumDeclaration> parse_enum();
    [[nodiscard]] ChainableResult<EnumMember> parse_enum_member(std::int64_t &counter);
    [[nodiscard]] std::vector<Token> collect_expression_tokens();
    [[nodiscard]] std::vector<Token> collect_enum_value_tokens();
    [[nodiscard]] ChainableResult<std::int64_t> evaluate_expression(const std::vector<Token> &expr_tokens);

    // Shunting Yard algorithm helpers
    [[nodiscard]] std::vector<Token> to_postfix(const std::vector<Token> &expr_tokens);
    [[nodiscard]] ChainableResult<std::int64_t> evaluate_postfix(const std::vector<Token> &postfix);
    [[nodiscard]] int operator_precedence(TokenType type) const;
    [[nodiscard]] bool is_left_associative(TokenType type) const;
    [[nodiscard]] bool is_operator(TokenType type) const;
    [[nodiscard]] bool is_unary_operator(TokenType type) const;

    [[nodiscard]] FormattableError make_error(SourcePosition pos, std::string message) const;

    const TextFormatter *format_;
    std::vector<Token> tokens_;
    std::size_t current_{0};
    std::unordered_map<std::string, std::int64_t> defined_values_{{"UCHAR_MAX", 255}}; // Symbol table for macro values
    const CParserContext *context_{nullptr};
};

} // namespace porytiles2
