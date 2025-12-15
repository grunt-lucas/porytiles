#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "porytiles2/utilities/c_parser/define_statement.hpp"
#include "porytiles2/utilities/c_parser/token.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

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
 * - parse_enums() for enum definitions
 * - parse_structs() for struct definitions
 */
class Parser {
  public:
    /**
     * @brief Constructs a parser for the given token stream.
     *
     * @param tokens The tokens to parse (typically from Lexer::lex())
     */
    explicit Parser(std::vector<Token> tokens);

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
     * @return A vector of DefineStatement on success, or an error on failure
     */
    [[nodiscard]] ChainableResult<std::vector<DefineStatement>> parse_defines();

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
    [[nodiscard]] std::vector<Token> collect_expression_tokens();
    [[nodiscard]] ChainableResult<std::int64_t> evaluate_expression(const std::vector<Token> &expr_tokens);

    // Shunting Yard algorithm helpers
    [[nodiscard]] std::vector<Token> to_postfix(const std::vector<Token> &expr_tokens);
    [[nodiscard]] ChainableResult<std::int64_t> evaluate_postfix(const std::vector<Token> &postfix);
    [[nodiscard]] int operator_precedence(TokenType type) const;
    [[nodiscard]] bool is_left_associative(TokenType type) const;
    [[nodiscard]] bool is_operator(TokenType type) const;
    [[nodiscard]] bool is_unary_operator(TokenType type) const;

    std::vector<Token> tokens_;
    std::size_t current_{0};
    std::unordered_map<std::string, std::int64_t> defined_values_; // Symbol table for macro values
};

} // namespace porytiles2
