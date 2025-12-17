#include "porytiles2/utilities/c_parser/lexer.hpp"

#include <cctype>
#include <charconv>
#include <unordered_map>

#include "porytiles2/utilities/c_parser/c_parser_context.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

namespace {

const std::unordered_map<std::string, TokenType> keywords = {
    {"define", TokenType::kw_define},
    {"undef", TokenType::kw_undef},
    {"include", TokenType::kw_include},
    {"ifdef", TokenType::kw_ifdef},
    {"ifndef", TokenType::kw_ifndef},
    {"if", TokenType::kw_if},
    {"else", TokenType::kw_else},
    {"elif", TokenType::kw_elif},
    {"endif", TokenType::kw_endif},
    {"defined", TokenType::kw_defined},
    {"pragma", TokenType::kw_pragma},
    {"enum", TokenType::kw_enum},
};

[[nodiscard]] bool is_identifier_start(char c)
{
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
}

[[nodiscard]] bool is_identifier_char(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

[[nodiscard]] bool is_digit(char c)
{
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

[[nodiscard]] bool is_hex_digit(char c)
{
    return std::isxdigit(static_cast<unsigned char>(c)) != 0;
}

[[nodiscard]] bool is_octal_digit(char c)
{
    return c >= '0' && c <= '7';
}

[[nodiscard]] bool is_binary_digit(char c)
{
    return c == '0' || c == '1';
}

} // namespace

std::string token_type_name(TokenType type)
{
    switch (type) {
    case TokenType::end_of_file:
        return "end_of_file";
    case TokenType::hash:
        return "#";
    case TokenType::kw_define:
        return "define";
    case TokenType::kw_undef:
        return "undef";
    case TokenType::kw_include:
        return "include";
    case TokenType::kw_ifdef:
        return "ifdef";
    case TokenType::kw_ifndef:
        return "ifndef";
    case TokenType::kw_if:
        return "if";
    case TokenType::kw_else:
        return "else";
    case TokenType::kw_elif:
        return "elif";
    case TokenType::kw_endif:
        return "endif";
    case TokenType::kw_defined:
        return "defined";
    case TokenType::kw_pragma:
        return "pragma";
    case TokenType::kw_enum:
        return "enum";
    case TokenType::identifier:
        return "identifier";
    case TokenType::integer_literal:
        return "integer_literal";
    case TokenType::string_literal:
        return "string_literal";
    case TokenType::char_literal:
        return "char_literal";
    case TokenType::plus:
        return "+";
    case TokenType::minus:
        return "-";
    case TokenType::star:
        return "*";
    case TokenType::slash:
        return "/";
    case TokenType::percent:
        return "%";
    case TokenType::ampersand:
        return "&";
    case TokenType::pipe:
        return "|";
    case TokenType::caret:
        return "^";
    case TokenType::tilde:
        return "~";
    case TokenType::exclaim:
        return "!";
    case TokenType::less:
        return "<";
    case TokenType::greater:
        return ">";
    case TokenType::equal:
        return "=";
    case TokenType::question:
        return "?";
    case TokenType::colon:
        return ":";
    case TokenType::less_less:
        return "<<";
    case TokenType::greater_greater:
        return ">>";
    case TokenType::ampersand_ampersand:
        return "&&";
    case TokenType::pipe_pipe:
        return "||";
    case TokenType::equal_equal:
        return "==";
    case TokenType::exclaim_equal:
        return "!=";
    case TokenType::less_equal:
        return "<=";
    case TokenType::greater_equal:
        return ">=";
    case TokenType::left_paren:
        return "(";
    case TokenType::right_paren:
        return ")";
    case TokenType::left_brace:
        return "{";
    case TokenType::right_brace:
        return "}";
    case TokenType::left_bracket:
        return "[";
    case TokenType::right_bracket:
        return "]";
    case TokenType::comma:
        return ",";
    case TokenType::semicolon:
        return ";";
    case TokenType::newline:
        return "newline";
    case TokenType::unknown:
        return "unknown";
    }
    return "unknown";
}

Lexer::Lexer(gsl::not_null<const TextFormatter *> format, std::string content)
    : format_{format}, content_{std::move(content)}
{
}

Lexer::Lexer(gsl::not_null<const TextFormatter *> format, std::string content, const CParserContext *context)
    : format_{format}, content_{std::move(content)}, context_{context}
{
}

FormattableError Lexer::make_error(SourcePosition pos, std::string message) const
{
    if (context_ != nullptr) {
        return context_->make_error(pos, std::move(message));
    }
    return FormattableError{format_->format("{}:{}: {}", pos.line, pos.column, message)};
}

ChainableResult<std::vector<Token>> Lexer::lex()
{
    std::vector<Token> tokens;

    while (!is_at_end()) {
        skip_whitespace_except_newline();

        if (is_at_end()) {
            break;
        }

        char c = peek();

        // Handle newlines
        if (c == '\n') {
            tokens.emplace_back(TokenType::newline, "\n", current_position());
            advance();
            continue;
        }

        // Handle comments
        if (c == '/') {
            if (peek_next() == '/') {
                skip_line_comment();
                continue;
            }
            if (peek_next() == '*') {
                auto result = skip_block_comment();
                if (!result.has_value()) {
                    return ChainableResult<std::vector<Token>>{result};
                }
                continue;
            }
        }

        // Handle preprocessor hash
        if (c == '#') {
            tokens.emplace_back(TokenType::hash, "#", current_position());
            advance();
            continue;
        }

        // Handle identifiers and keywords
        if (is_identifier_start(c)) {
            tokens.push_back(consume_identifier_or_keyword());
            continue;
        }

        // Handle numbers
        if (is_digit(c)) {
            auto result = consume_number();
            if (!result.has_value()) {
                return ChainableResult<std::vector<Token>>{result};
            }
            tokens.push_back(std::move(result).value());
            continue;
        }

        // Handle string literals
        if (c == '"') {
            auto result = consume_string();
            if (!result.has_value()) {
                return ChainableResult<std::vector<Token>>{result};
            }
            tokens.push_back(std::move(result).value());
            continue;
        }

        // Handle operators and delimiters
        tokens.push_back(consume_operator());
    }

    tokens.emplace_back(TokenType::end_of_file, "", current_position());
    return tokens;
}

char Lexer::peek() const
{
    if (is_at_end()) {
        return '\0';
    }
    return content_[current_];
}

char Lexer::peek_next() const
{
    if (current_ + 1 >= content_.size()) {
        return '\0';
    }
    return content_[current_ + 1];
}

char Lexer::advance()
{
    if (is_at_end()) {
        return '\0';
    }
    char c = content_[current_];
    ++current_;
    if (c == '\n') {
        ++line_;
        column_ = 1;
    }
    else {
        ++column_;
    }
    return c;
}

bool Lexer::is_at_end() const
{
    return current_ >= content_.size();
}

void Lexer::skip_whitespace_except_newline()
{
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        }
        else {
            break;
        }
    }
}

void Lexer::skip_line_comment()
{
    // Skip the //
    advance();
    advance();

    while (!is_at_end() && peek() != '\n') {
        advance();
    }
    // Don't consume the newline - it's significant for preprocessor
}

ChainableResult<void> Lexer::skip_block_comment()
{
    SourcePosition start_pos = current_position();

    // Skip the /*
    advance();
    advance();

    while (!is_at_end()) {
        if (peek() == '*' && peek_next() == '/') {
            advance(); // *
            advance(); // /
            return {};
        }
        advance();
    }

    return make_error(start_pos, "unterminated block comment");
}

Token Lexer::consume_identifier_or_keyword()
{
    SourcePosition start_pos = current_position();
    std::string text;

    while (!is_at_end() && is_identifier_char(peek())) {
        text += advance();
    }

    auto it = keywords.find(text);
    if (it != keywords.end()) {
        return Token{it->second, std::move(text), start_pos};
    }

    return Token{TokenType::identifier, std::move(text), start_pos};
}

ChainableResult<Token> Lexer::consume_number()
{
    SourcePosition start_pos = current_position();
    std::string text;

    // Check for hex, octal, or binary prefix
    if (peek() == '0' && !is_at_end()) {
        text += advance();
        char next = peek();

        // Hexadecimal: 0x or 0X
        if (next == 'x' || next == 'X') {
            text += advance();
            if (!is_hex_digit(peek())) {
                return make_error(start_pos, format_->format("invalid hexadecimal literal '{}'", text));
            }
            while (!is_at_end() && is_hex_digit(peek())) {
                text += advance();
            }
            // Skip optional integer suffix (u, U, l, L, ll, LL, etc.)
            while (!is_at_end() && (peek() == 'u' || peek() == 'U' || peek() == 'l' || peek() == 'L')) {
                text += advance();
            }
            std::int64_t value = 0;
            auto [ptr, ec] = std::from_chars(text.data() + 2, text.data() + text.size(), value, 16);
            if (ec != std::errc{}) {
                return make_error(start_pos, format_->format("invalid hexadecimal literal '{}'", text));
            }
            return Token{std::move(text), value, start_pos};
        }

        // Binary: 0b or 0B
        if (next == 'b' || next == 'B') {
            text += advance();
            if (!is_binary_digit(peek())) {
                return make_error(start_pos, format_->format("invalid binary literal '{}'", text));
            }
            while (!is_at_end() && is_binary_digit(peek())) {
                text += advance();
            }
            // Skip optional integer suffix
            while (!is_at_end() && (peek() == 'u' || peek() == 'U' || peek() == 'l' || peek() == 'L')) {
                text += advance();
            }
            std::int64_t value = 0;
            auto [ptr, ec] = std::from_chars(text.data() + 2, text.data() + text.size(), value, 2);
            if (ec != std::errc{}) {
                return make_error(start_pos, format_->format("invalid binary literal '{}'", text));
            }
            return Token{std::move(text), value, start_pos};
        }

        // Octal: starts with 0 followed by octal digits
        if (is_octal_digit(next)) {
            while (!is_at_end() && is_octal_digit(peek())) {
                text += advance();
            }
            // Skip optional integer suffix
            while (!is_at_end() && (peek() == 'u' || peek() == 'U' || peek() == 'l' || peek() == 'L')) {
                text += advance();
            }
            std::int64_t value = 0;
            auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, 8);
            if (ec != std::errc{}) {
                return make_error(start_pos, format_->format("invalid octal literal '{}'", text));
            }
            return Token{std::move(text), value, start_pos};
        }

        // Just a single 0
        return Token{std::move(text), 0, start_pos};
    }

    // Decimal number
    while (!is_at_end() && is_digit(peek())) {
        text += advance();
    }
    // Skip optional integer suffix
    while (!is_at_end() && (peek() == 'u' || peek() == 'U' || peek() == 'l' || peek() == 'L')) {
        text += advance();
    }

    std::int64_t value = 0;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, 10);
    if (ec != std::errc{}) {
        return make_error(start_pos, format_->format("invalid decimal literal '{}'", text));
    }
    return Token{std::move(text), value, start_pos};
}

ChainableResult<Token> Lexer::consume_string()
{
    SourcePosition start_pos = current_position();
    std::string text;
    std::string value;

    text += advance(); // Opening quote

    while (!is_at_end() && peek() != '"') {
        char c = peek();

        // Check for unterminated string (newline without escape)
        if (c == '\n') {
            return make_error(start_pos, "unterminated string literal");
        }

        // Handle escape sequences
        if (c == '\\' && !is_at_end()) {
            text += advance(); // backslash
            if (!is_at_end()) {
                char escaped = advance();
                text += escaped;
                switch (escaped) {
                case 'n':
                    value += '\n';
                    break;
                case 't':
                    value += '\t';
                    break;
                case 'r':
                    value += '\r';
                    break;
                case '\\':
                    value += '\\';
                    break;
                case '"':
                    value += '"';
                    break;
                case '0':
                    value += '\0';
                    break;
                default:
                    value += escaped; // Unknown escape, keep as-is
                    break;
                }
            }
        }
        else {
            text += c;
            value += c;
            advance();
        }
    }

    if (is_at_end()) {
        return make_error(start_pos, "unterminated string literal");
    }

    text += advance(); // Closing quote

    // Create a token that stores the string value in text_
    // For string literals, we store the unquoted, unescaped value
    return Token{TokenType::string_literal, std::move(value), start_pos};
}

Token Lexer::consume_operator()
{
    SourcePosition start_pos = current_position();
    char c = advance();

    switch (c) {
    case '+':
        return Token{TokenType::plus, "+", start_pos};
    case '-':
        return Token{TokenType::minus, "-", start_pos};
    case '*':
        return Token{TokenType::star, "*", start_pos};
    case '/':
        return Token{TokenType::slash, "/", start_pos};
    case '%':
        return Token{TokenType::percent, "%", start_pos};
    case '~':
        return Token{TokenType::tilde, "~", start_pos};
    case '^':
        return Token{TokenType::caret, "^", start_pos};
    case '?':
        return Token{TokenType::question, "?", start_pos};
    case ':':
        return Token{TokenType::colon, ":", start_pos};
    case '(':
        return Token{TokenType::left_paren, "(", start_pos};
    case ')':
        return Token{TokenType::right_paren, ")", start_pos};
    case '{':
        return Token{TokenType::left_brace, "{", start_pos};
    case '}':
        return Token{TokenType::right_brace, "}", start_pos};
    case '[':
        return Token{TokenType::left_bracket, "[", start_pos};
    case ']':
        return Token{TokenType::right_bracket, "]", start_pos};
    case ',':
        return Token{TokenType::comma, ",", start_pos};
    case ';':
        return Token{TokenType::semicolon, ";", start_pos};

    case '<':
        if (peek() == '<') {
            advance();
            return Token{TokenType::less_less, "<<", start_pos};
        }
        if (peek() == '=') {
            advance();
            return Token{TokenType::less_equal, "<=", start_pos};
        }
        return Token{TokenType::less, "<", start_pos};

    case '>':
        if (peek() == '>') {
            advance();
            return Token{TokenType::greater_greater, ">>", start_pos};
        }
        if (peek() == '=') {
            advance();
            return Token{TokenType::greater_equal, ">=", start_pos};
        }
        return Token{TokenType::greater, ">", start_pos};

    case '&':
        if (peek() == '&') {
            advance();
            return Token{TokenType::ampersand_ampersand, "&&", start_pos};
        }
        return Token{TokenType::ampersand, "&", start_pos};

    case '|':
        if (peek() == '|') {
            advance();
            return Token{TokenType::pipe_pipe, "||", start_pos};
        }
        return Token{TokenType::pipe, "|", start_pos};

    case '=':
        if (peek() == '=') {
            advance();
            return Token{TokenType::equal_equal, "==", start_pos};
        }
        return Token{TokenType::equal, "=", start_pos};

    case '!':
        if (peek() == '=') {
            advance();
            return Token{TokenType::exclaim_equal, "!=", start_pos};
        }
        return Token{TokenType::exclaim, "!", start_pos};

    default:
        return Token{TokenType::unknown, std::string(1, c), start_pos};
    }
}

SourcePosition Lexer::current_position() const
{
    return SourcePosition{line_, column_};
}

} // namespace porytiles2
