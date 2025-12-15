#pragma once

#include <cstdint>
#include <string>

#include "porytiles2/utilities/c_parser/source_position.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Enumeration of token types recognized by the C parser lexer.
 *
 * @details
 * TokenType defines all token categories the lexer can produce. The initial implementation focuses on tokens needed for
 * #define parsing, but additional token types are included for future expansion to support functions, conditionals, and
 * other C constructs.
 */
enum class TokenType : std::uint8_t {
    // === End of input ===
    end_of_file,

    // === Preprocessor directives ===
    hash,       // #
    kw_define,  // define
    kw_undef,   // undef (reserved for future)
    kw_include, // include (reserved for future)
    kw_ifdef,   // ifdef (reserved for future)
    kw_ifndef,  // ifndef (reserved for future)
    kw_if,      // if (reserved for future)
    kw_else,    // else (reserved for future)
    kw_elif,    // elif (reserved for future)
    kw_endif,   // endif (reserved for future)
    kw_defined, // defined (reserved for future)
    kw_pragma,  // pragma (reserved for future)

    // === Literals ===
    identifier,      // variable/macro names
    integer_literal, // decimal, hex, octal, binary integers
    string_literal,  // "..." strings
    char_literal,    // '...' character literals (reserved for future)

    // === Operators ===
    plus,      // +
    minus,     // -
    star,      // *
    slash,     // /
    percent,   // %
    ampersand, // &
    pipe,      // |
    caret,     // ^
    tilde,     // ~
    exclaim,   // !
    less,      // <
    greater,   // >
    equal,     // =
    question,  // ? (reserved for future)
    colon,     // : (reserved for future)

    // === Compound operators ===
    less_less,           // <<
    greater_greater,     // >>
    ampersand_ampersand, // && (reserved for future)
    pipe_pipe,           // || (reserved for future)
    equal_equal,         // == (reserved for future)
    exclaim_equal,       // != (reserved for future)
    less_equal,          // <= (reserved for future)
    greater_equal,       // >= (reserved for future)

    // === Delimiters ===
    left_paren,    // (
    right_paren,   // )
    left_brace,    // { (reserved for future)
    right_brace,   // } (reserved for future)
    left_bracket,  // [ (reserved for future)
    right_bracket, // ] (reserved for future)
    comma,         // ,
    semicolon,     // ; (reserved for future)

    // === Special ===
    newline, // Significant for preprocessor directives
    unknown, // Unrecognized character
};

/**
 * @brief Returns a human-readable name for a token type.
 *
 * @param type The token type
 * @return A string describing the token type
 */
[[nodiscard]] std::string token_type_name(TokenType type);

/**
 * @brief Represents a lexical token from C source code.
 *
 * @details
 * Token captures the type, textual representation, parsed value (if applicable), and source position of a lexical unit.
 * For integer literals, the parsed numeric value is stored in int_value_. For identifiers and string literals, the text
 * representation is used.
 *
 * @invariant type_ is always set to a valid TokenType
 * @invariant position_ contains valid 1-based line and column numbers
 */
class Token {
  public:
    /**
     * @brief Constructs a token with the given type, text, and position.
     *
     * @param type The token type
     * @param text The raw text of the token
     * @param position The source position where the token starts
     */
    Token(TokenType type, std::string text, SourcePosition position)
        : type_{type}, text_{std::move(text)}, position_{position}
    {
    }

    /**
     * @brief Constructs an integer literal token with a parsed value.
     *
     * @param text The raw text of the token
     * @param int_value The parsed integer value
     * @param position The source position where the token starts
     */
    Token(std::string text, std::int64_t int_value, SourcePosition position)
        : type_{TokenType::integer_literal}, text_{std::move(text)}, position_{position}, int_value_{int_value}
    {
    }

    /**
     * @brief Returns the token type.
     *
     * @return The token type
     */
    [[nodiscard]] TokenType type() const
    {
        return type_;
    }

    /**
     * @brief Returns the raw text of the token.
     *
     * @return A const reference to the token text
     */
    [[nodiscard]] const std::string &text() const
    {
        return text_;
    }

    /**
     * @brief Returns the source position where the token starts.
     *
     * @return A const reference to the source position
     */
    [[nodiscard]] const SourcePosition &position() const
    {
        return position_;
    }

    /**
     * @brief Returns the integer value for integer_literal tokens.
     *
     * @pre type() == TokenType::integer_literal
     * @return The parsed integer value
     */
    [[nodiscard]] std::int64_t int_value() const
    {
        assert_or_panic(type_ == TokenType::integer_literal, "int_value() called on non-integer token");
        return int_value_;
    }

    /**
     * @brief Checks if this token is of the specified type.
     *
     * @param type The token type to check against
     * @return True if this token's type matches
     */
    [[nodiscard]] bool is(TokenType type) const
    {
        return type_ == type;
    }

    /**
     * @brief Checks if this token is any of the specified types.
     *
     * @tparam Types Variadic token types
     * @param types The token types to check against
     * @return True if this token's type matches any of the given types
     */
    template <typename... Types>
    [[nodiscard]] bool is_any_of(Types... types) const
    {
        return ((type_ == types) || ...);
    }

  private:
    TokenType type_;
    std::string text_;
    SourcePosition position_;
    std::int64_t int_value_{0}; // Only valid for integer_literal
};

} // namespace porytiles2
