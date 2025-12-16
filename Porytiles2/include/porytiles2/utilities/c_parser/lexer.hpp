#pragma once

#include <string>
#include <vector>

#include "porytiles2/utilities/c_parser/source_position.hpp"
#include "porytiles2/utilities/c_parser/token.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

class CParserContext;

/**
 * @brief Lexical analyzer for C/C++ source code.
 *
 * @details
 * Lexer converts a string of C/C++ source code into a sequence of tokens. It handles:
 * - Preprocessor directives (#define, #ifdef, etc.)
 * - Identifiers and keywords
 * - Integer literals (decimal, hexadecimal, octal, binary)
 * - String literals
 * - Operators and delimiters
 * - Comments (// and multi-line, which are skipped)
 * - Whitespace (skipped except for newlines which are significant for preprocessor)
 *
 * The lexer tracks source positions for error reporting. It does not perform any file I/O; the caller is responsible
 * for reading file contents and passing them as a string.
 *
 * Example usage:
 * @code
 * Lexer lexer{"#define FOO 123\n#define BAR 0x456"};
 * auto result = lexer.lex();
 * if (result.has_value()) {
 *     for (const auto& token : result.value()) {
 *         // process tokens
 *     }
 * }
 * @endcode
 */
class Lexer {
  public:
    /**
     * @brief Constructs a lexer for the given source content.
     *
     * @details
     * This constructor creates a lexer without a CParserContext. Errors will be formatted as simple "line:col: message"
     * strings without source context highlighting.
     *
     * @param content The C/C++ source code to tokenize
     */
    explicit Lexer(std::string content);

    /**
     * @brief Constructs a lexer with a context for rich error formatting.
     *
     * @details
     * This constructor creates a lexer with a CParserContext that enables rich error formatting with source context
     * highlighting via FileHighlightPrinter. The context must outlive the lexer.
     *
     * @param content The C/C++ source code to tokenize
     * @param context The parser context for rich error formatting (non-owning)
     */
    Lexer(std::string content, const CParserContext *context);

    /**
     * @brief Tokenizes the entire source content.
     *
     * @details
     * Processes the source content and returns a vector of tokens. The token stream always ends with an end_of_file
     * token. If an error is encountered (e.g., unterminated string), returns a ChainableResult containing an error with
     * source position information.
     *
     * When constructed with a CParserContext, errors include source code context with highlighted error locations. When
     * constructed without a context, errors are simple "line:col: message" format.
     *
     * @return A vector of tokens on success, or an error on failure
     */
    [[nodiscard]] ChainableResult<std::vector<Token>> lex();

  private:
    [[nodiscard]] char peek() const;
    [[nodiscard]] char peek_next() const;
    char advance();
    [[nodiscard]] bool is_at_end() const;
    void skip_whitespace_except_newline();
    void skip_line_comment();
    [[nodiscard]] ChainableResult<void> skip_block_comment();

    [[nodiscard]] Token consume_identifier_or_keyword();
    [[nodiscard]] ChainableResult<Token> consume_number();
    [[nodiscard]] ChainableResult<Token> consume_string();
    [[nodiscard]] Token consume_operator();

    [[nodiscard]] SourcePosition current_position() const;
    [[nodiscard]] FormattableError make_error(SourcePosition pos, std::string message) const;

    std::string content_;
    std::size_t current_{0};
    std::size_t line_{1};
    std::size_t column_{1};
    const CParserContext *context_{nullptr};
};

} // namespace porytiles2
