#include "porytiles2/utilities/c_parser/parser.hpp"

#include <stack>

#include "fmt/format.h"

namespace porytiles2 {

Parser::Parser(std::vector<Token> tokens) : tokens_{std::move(tokens)} {}

ChainableResult<std::vector<DefineStatement>, CParserError> Parser::parse_defines()
{
    std::vector<DefineStatement> defines;

    while (!is_at_end()) {
        // Look for # token
        if (check(TokenType::hash)) {
            advance(); // consume #

            // Check if this is a #define
            if (check(TokenType::kw_define)) {
                auto result = parse_define();
                if (!result.has_value()) {
                    return ChainableResult<std::vector<DefineStatement>, CParserError>{result};
                }
                defines.push_back(std::move(result).value());
            }
            else {
                // Skip other preprocessor directives
                skip_to_next_line();
            }
        }
        else {
            // Skip non-preprocessor tokens
            advance();
        }
    }

    return defines;
}

ChainableResult<std::vector<EnumDeclaration>, CParserError> Parser::parse_enums()
{
    std::vector<EnumDeclaration> enums;

    // Reset position to beginning (allows calling both parse_defines and parse_enums)
    current_ = 0;

    while (!is_at_end()) {
        if (check(TokenType::kw_enum)) {
            auto result = parse_enum();
            if (!result.has_value()) {
                return ChainableResult<std::vector<EnumDeclaration>, CParserError>{result};
            }
            enums.push_back(std::move(result).value());
        }
        else {
            advance();
        }
    }

    return enums;
}

ChainableResult<EnumDeclaration, CParserError> Parser::parse_enum()
{
    SourcePosition enum_pos = peek().position();
    advance(); // consume 'enum'

    // Check for optional enum name
    std::optional<std::string> enum_name;
    if (check(TokenType::identifier)) {
        enum_name = peek().text();
        advance();
    }

    // Expect opening brace
    if (!check(TokenType::left_brace)) {
        return CParserError{peek().position(), "expected '{' after 'enum'"};
    }
    advance(); // consume '{'

    // Parse members
    std::vector<EnumMember> members;
    std::int64_t counter = 0;

    while (!is_at_end() && !check(TokenType::right_brace)) {
        // Skip newlines
        while (check(TokenType::newline)) {
            advance();
        }

        if (check(TokenType::right_brace)) {
            break;
        }

        auto member_result = parse_enum_member(counter);
        if (!member_result.has_value()) {
            return ChainableResult<EnumDeclaration, CParserError>{member_result};
        }
        members.push_back(std::move(member_result).value());

        // Skip trailing comma (optional for last member)
        if (check(TokenType::comma)) {
            advance();
        }

        // Skip newlines after comma
        while (check(TokenType::newline)) {
            advance();
        }
    }

    // Expect closing brace
    if (!check(TokenType::right_brace)) {
        return CParserError{peek().position(), "expected '}' to close enum"};
    }
    advance(); // consume '}'

    // Skip optional semicolon
    if (check(TokenType::semicolon)) {
        advance();
    }

    if (enum_name.has_value()) {
        return EnumDeclaration{std::move(enum_name).value(), std::move(members), enum_pos};
    }
    return EnumDeclaration{std::move(members), enum_pos};
}

ChainableResult<EnumMember, CParserError> Parser::parse_enum_member(std::int64_t &counter)
{
    // Expect identifier
    if (!check(TokenType::identifier)) {
        return CParserError{peek().position(), "expected identifier for enum member"};
    }

    std::string name = peek().text();
    SourcePosition member_pos = peek().position();
    advance(); // consume identifier

    // Check for explicit value assignment
    bool has_explicit = false;
    if (check(TokenType::equal)) {
        advance(); // consume '='
        has_explicit = true;

        // Collect expression tokens until comma, right_brace, or newline
        std::vector<Token> expr_tokens = collect_enum_value_tokens();

        if (expr_tokens.empty()) {
            return CParserError{member_pos, fmt::format("expected expression after '=' for enum member '{}'", name)};
        }

        auto eval_result = evaluate_expression(expr_tokens);
        if (!eval_result.has_value()) {
            return ChainableResult<EnumMember, CParserError>{
                CParserError{member_pos, fmt::format("failed to evaluate expression for enum member '{}'", name)},
                eval_result};
        }

        counter = eval_result.value();
    }

    EnumMember member{std::move(name), counter, has_explicit, member_pos};
    counter++; // Increment for next member
    return member;
}

std::vector<Token> Parser::collect_enum_value_tokens()
{
    std::vector<Token> expr_tokens;

    while (!is_at_end() && !check(TokenType::comma) && !check(TokenType::right_brace) && !check(TokenType::newline)) {
        expr_tokens.push_back(peek());
        advance();
    }

    return expr_tokens;
}

const Token &Parser::peek() const
{
    if (is_at_end()) {
        return tokens_.back(); // Should be end_of_file
    }
    return tokens_[current_];
}

const Token &Parser::peek_next() const
{
    if (current_ + 1 >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[current_ + 1];
}

const Token &Parser::advance()
{
    if (!is_at_end()) {
        ++current_;
    }
    return tokens_[current_ - 1];
}

bool Parser::is_at_end() const
{
    return current_ >= tokens_.size() || tokens_[current_].is(TokenType::end_of_file);
}

bool Parser::check(TokenType type) const
{
    if (is_at_end()) {
        return type == TokenType::end_of_file;
    }
    return peek().is(type);
}

bool Parser::match(TokenType type)
{
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::skip_to_next_line()
{
    while (!is_at_end() && !check(TokenType::newline)) {
        advance();
    }
    if (check(TokenType::newline)) {
        advance(); // consume the newline
    }
}

bool Parser::is_at_line_end() const
{
    return is_at_end() || check(TokenType::newline);
}

ChainableResult<DefineStatement, CParserError> Parser::parse_define()
{
    SourcePosition define_pos = peek().position();
    advance(); // consume 'define'

    // Skip whitespace (already handled by lexer, but newlines are significant)

    // Expect identifier for macro name
    if (!check(TokenType::identifier)) {
        return CParserError{peek().position(), "expected identifier after '#define'"};
    }

    std::string name = peek().text();
    SourcePosition name_pos = peek().position();
    std::size_t name_end_column = name_pos.column + name.size();
    advance(); // consume identifier

    // Check for parametric macro (function-like macro)
    // These have ( immediately after the name with no space
    // We detect this by checking if the ( starts at the column immediately after the identifier
    if (check(TokenType::left_paren)) {
        SourcePosition paren_pos = peek().position();
        // If the ( is immediately after the identifier (same line, adjacent column), it's a parametric macro
        if (paren_pos.line == name_pos.line && paren_pos.column == name_end_column) {
            // This is a parametric macro - skip it
            skip_to_next_line();
            return DefineStatement{std::move(name), define_pos}; // Return as flag define
        }
        // Otherwise, the ( starts an expression - fall through to expression evaluation
    }

    // Check for end of line (flag-like define with no value)
    if (is_at_line_end()) {
        if (check(TokenType::newline)) {
            advance();
        }
        return DefineStatement{std::move(name), define_pos};
    }

    // Check for string literal value
    if (check(TokenType::string_literal)) {
        std::string value = peek().text();
        advance();
        skip_to_next_line();
        return DefineStatement{std::move(name), std::move(value), define_pos};
    }

    // Otherwise, collect and evaluate expression
    std::vector<Token> expr_tokens = collect_expression_tokens();

    if (expr_tokens.empty()) {
        // No expression, treat as flag define
        return DefineStatement{std::move(name), define_pos};
    }

    auto result = evaluate_expression(expr_tokens);
    if (!result.has_value()) {
        return ChainableResult<DefineStatement, CParserError>{
            CParserError{define_pos, fmt::format("failed to evaluate expression for '#define {}'", name)}, result};
    }

    std::int64_t value = result.value();

    // Store in symbol table for later references
    defined_values_[name] = value;

    return DefineStatement{std::move(name), value, define_pos};
}

std::vector<Token> Parser::collect_expression_tokens()
{
    std::vector<Token> expr_tokens;

    while (!is_at_line_end()) {
        expr_tokens.push_back(peek());
        advance();
    }

    // Consume the newline if present
    if (check(TokenType::newline)) {
        advance();
    }

    return expr_tokens;
}

ChainableResult<std::int64_t, CParserError> Parser::evaluate_expression(const std::vector<Token> &expr_tokens)
{
    if (expr_tokens.empty()) {
        return CParserError{SourcePosition{}, "empty expression"};
    }

    // Convert to postfix notation using Shunting Yard
    std::vector<Token> postfix = to_postfix(expr_tokens);

    // Evaluate the postfix expression
    return evaluate_postfix(postfix);
}

std::vector<Token> Parser::to_postfix(const std::vector<Token> &expr_tokens)
{
    std::vector<Token> output;
    std::stack<Token> operators;

    bool expect_operand = true; // Track if we expect an operand (for unary operators)

    for (std::size_t i = 0; i < expr_tokens.size(); ++i) {
        const Token &token = expr_tokens[i];

        if (token.is(TokenType::integer_literal) || token.is(TokenType::identifier)) {
            output.push_back(token);
            expect_operand = false;
        }
        else if (token.is(TokenType::left_paren)) {
            operators.push(token);
            expect_operand = true;
        }
        else if (token.is(TokenType::right_paren)) {
            while (!operators.empty() && !operators.top().is(TokenType::left_paren)) {
                output.push_back(operators.top());
                operators.pop();
            }
            if (!operators.empty() && operators.top().is(TokenType::left_paren)) {
                operators.pop(); // Discard the left paren
            }
            expect_operand = false;
        }
        else if (is_operator(token.type())) {
            // Handle unary operators (-, ~, !)
            if (expect_operand && is_unary_operator(token.type())) {
                // Create a special unary token by prefixing with 'u'
                // We'll handle this in evaluation
                Token unary_token{token.type(), "u" + token.text(), token.position()};
                operators.push(unary_token);
            }
            else {
                // Binary operator
                while (!operators.empty() && !operators.top().is(TokenType::left_paren) &&
                       is_operator(operators.top().type())) {
                    int top_prec = operator_precedence(operators.top().type());
                    int curr_prec = operator_precedence(token.type());

                    if (top_prec < curr_prec || (top_prec == curr_prec && is_left_associative(token.type()))) {
                        output.push_back(operators.top());
                        operators.pop();
                    }
                    else {
                        break;
                    }
                }
                operators.push(token);
                expect_operand = true;
            }
        }
        // Skip unknown tokens
    }

    // Pop remaining operators
    while (!operators.empty()) {
        if (!operators.top().is(TokenType::left_paren)) {
            output.push_back(operators.top());
        }
        operators.pop();
    }

    return output;
}

ChainableResult<std::int64_t, CParserError> Parser::evaluate_postfix(const std::vector<Token> &postfix)
{
    std::stack<std::int64_t> values;

    for (const Token &token : postfix) {
        if (token.is(TokenType::integer_literal)) {
            values.push(token.int_value());
        }
        else if (token.is(TokenType::identifier)) {
            // Look up in symbol table
            auto it = defined_values_.find(token.text());
            if (it != defined_values_.end()) {
                values.push(it->second);
            }
            else {
                // Unknown identifier - could be an error or treat as 0
                return CParserError{token.position(), fmt::format("unknown identifier '{}'", token.text())};
            }
        }
        else if (is_operator(token.type())) {
            // Check if it's a unary operator (text starts with 'u')
            if (token.text().size() > 1 && token.text()[0] == 'u') {
                if (values.empty()) {
                    return CParserError{
                        token.position(), fmt::format("unary operator '{}' missing operand", token.text().substr(1))};
                }
                std::int64_t operand = values.top();
                values.pop();

                std::int64_t result = 0;
                switch (token.type()) {
                case TokenType::minus:
                    result = -operand;
                    break;
                case TokenType::tilde:
                    result = ~operand;
                    break;
                case TokenType::exclaim:
                    result = operand == 0 ? 1 : 0;
                    break;
                default:
                    return CParserError{token.position(), fmt::format("unknown unary operator '{}'", token.text())};
                }
                values.push(result);
            }
            else {
                // Binary operator
                if (values.size() < 2) {
                    return CParserError{
                        token.position(), fmt::format("binary operator '{}' missing operands", token.text())};
                }
                std::int64_t right = values.top();
                values.pop();
                std::int64_t left = values.top();
                values.pop();

                std::int64_t result = 0;
                switch (token.type()) {
                case TokenType::plus:
                    result = left + right;
                    break;
                case TokenType::minus:
                    result = left - right;
                    break;
                case TokenType::star:
                    result = left * right;
                    break;
                case TokenType::slash:
                    if (right == 0) {
                        return CParserError{token.position(), "division by zero"};
                    }
                    result = left / right;
                    break;
                case TokenType::percent:
                    if (right == 0) {
                        return CParserError{token.position(), "modulo by zero"};
                    }
                    result = left % right;
                    break;
                case TokenType::ampersand:
                    result = left & right;
                    break;
                case TokenType::pipe:
                    result = left | right;
                    break;
                case TokenType::caret:
                    result = left ^ right;
                    break;
                case TokenType::less_less:
                    result = left << right;
                    break;
                case TokenType::greater_greater:
                    result = left >> right;
                    break;
                default:
                    return CParserError{token.position(), fmt::format("unknown binary operator '{}'", token.text())};
                }
                values.push(result);
            }
        }
    }

    if (values.empty()) {
        return CParserError{SourcePosition{}, "expression evaluated to no value"};
    }

    return values.top();
}

int Parser::operator_precedence(TokenType type) const
{
    // Lower number = higher precedence (evaluated first)
    // Based on C operator precedence
    switch (type) {
    case TokenType::star:
    case TokenType::slash:
    case TokenType::percent:
        return 3;
    case TokenType::plus:
    case TokenType::minus:
        return 4;
    case TokenType::less_less:
    case TokenType::greater_greater:
        return 5;
    case TokenType::less:
    case TokenType::greater:
    case TokenType::less_equal:
    case TokenType::greater_equal:
        return 6;
    case TokenType::equal_equal:
    case TokenType::exclaim_equal:
        return 7;
    case TokenType::ampersand:
        return 8;
    case TokenType::caret:
        return 9;
    case TokenType::pipe:
        return 10;
    case TokenType::ampersand_ampersand:
        return 11;
    case TokenType::pipe_pipe:
        return 12;
    default:
        return 99; // Lowest precedence for unknown
    }
}

bool Parser::is_left_associative(TokenType type) const
{
    // All our binary operators are left-associative
    return true;
}

bool Parser::is_operator(TokenType type) const
{
    switch (type) {
    case TokenType::plus:
    case TokenType::minus:
    case TokenType::star:
    case TokenType::slash:
    case TokenType::percent:
    case TokenType::ampersand:
    case TokenType::pipe:
    case TokenType::caret:
    case TokenType::tilde:
    case TokenType::exclaim:
    case TokenType::less:
    case TokenType::greater:
    case TokenType::less_less:
    case TokenType::greater_greater:
    case TokenType::less_equal:
    case TokenType::greater_equal:
    case TokenType::equal_equal:
    case TokenType::exclaim_equal:
    case TokenType::ampersand_ampersand:
    case TokenType::pipe_pipe:
        return true;
    default:
        return false;
    }
}

bool Parser::is_unary_operator(TokenType type) const
{
    switch (type) {
    case TokenType::minus:   // Unary negation
    case TokenType::tilde:   // Bitwise NOT
    case TokenType::exclaim: // Logical NOT
    case TokenType::plus:    // Unary plus (no-op but valid)
        return true;
    default:
        return false;
    }
}

} // namespace porytiles2
