#include "porytiles2/utilities/c_parser/parser.hpp"

#include <stack>

#include "porytiles2/utilities/c_parser/c_parser_context.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

namespace {

/**
 * @brief Skips tokens until finding a matching closing brace, handling nested braces.
 *
 * @param tokens The token vector
 * @param start Position of the opening brace
 * @return Position after the closing brace, or tokens.size() if not found
 */
[[nodiscard]] std::size_t skip_balanced_braces(const std::vector<Token> &tokens, std::size_t start)
{
    if (start >= tokens.size() || !tokens[start].is(TokenType::left_brace)) {
        return start;
    }

    std::size_t pos = start + 1;
    int depth = 1;

    while (pos < tokens.size() && depth > 0) {
        if (tokens[pos].is(TokenType::left_brace)) {
            ++depth;
        }
        else if (tokens[pos].is(TokenType::right_brace)) {
            --depth;
        }
        ++pos;
    }

    return pos;
}

/**
 * @brief Collects all tokens between braces, handling nested braces.
 *
 * @param tokens The token vector
 * @param start Position of the opening brace
 * @return Vector of tokens between the braces (exclusive of braces themselves)
 */
[[nodiscard]] std::vector<Token> collect_brace_contents(const std::vector<Token> &tokens, std::size_t start)
{
    std::vector<Token> contents;

    if (start >= tokens.size() || !tokens[start].is(TokenType::left_brace)) {
        return contents;
    }

    std::size_t pos = start + 1;
    int depth = 1;

    while (pos < tokens.size() && depth > 0) {
        if (tokens[pos].is(TokenType::left_brace)) {
            ++depth;
        }
        else if (tokens[pos].is(TokenType::right_brace)) {
            --depth;
            if (depth == 0) {
                break;
            }
        }
        contents.push_back(tokens[pos]);
        ++pos;
    }

    return contents;
}

/**
 * @brief Extracts identifier names from a comma-separated list in brace contents.
 *
 * @param brace_contents Tokens from inside the braces
 * @return Vector of identifier names in order
 */
[[nodiscard]] std::vector<std::string> extract_identifier_elements(const std::vector<Token> &brace_contents)
{
    std::vector<std::string> elements;

    for (const auto &token : brace_contents) {
        if (token.is(TokenType::identifier)) {
            elements.push_back(token.text());
        }
        // Skip commas, newlines, and other tokens
    }

    return elements;
}

/**
 * @brief Skips tokens until finding a matching closing parenthesis.
 *
 * @param tokens The token vector
 * @param start Position of the opening parenthesis
 * @return Position after the closing parenthesis, or tokens.size() if not found
 */
[[nodiscard]] std::size_t skip_balanced_parens(const std::vector<Token> &tokens, std::size_t start)
{
    if (start >= tokens.size() || !tokens[start].is(TokenType::left_paren)) {
        return start;
    }

    std::size_t pos = start + 1;
    int depth = 1;

    while (pos < tokens.size() && depth > 0) {
        if (tokens[pos].is(TokenType::left_paren)) {
            ++depth;
        }
        else if (tokens[pos].is(TokenType::right_paren)) {
            --depth;
        }
        ++pos;
    }

    return pos;
}

} // namespace

FormattableError Parser::make_error(SourcePosition pos, std::string message) const
{
    if (context_ != nullptr) {
        return context_->make_error(pos, std::move(message));
    }
    return FormattableError{format_->format("{}:{}: {}", pos.line, pos.column, message)};
}

ChainableResult<std::vector<DefineStatement>> Parser::parse_defines()
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
                    return ChainableResult<std::vector<DefineStatement>>{result};
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

ChainableResult<std::vector<EnumDeclaration>> Parser::parse_enums()
{
    std::vector<EnumDeclaration> enums;

    // Reset position to beginning (allows calling both parse_defines and parse_enums)
    current_ = 0;

    while (!is_at_end()) {
        if (check(TokenType::kw_enum)) {
            auto result = parse_enum();
            if (!result.has_value()) {
                return ChainableResult<std::vector<EnumDeclaration>>{result};
            }
            enums.push_back(std::move(result).value());
        }
        else {
            advance();
        }
    }

    return enums;
}

ChainableResult<EnumDeclaration> Parser::parse_enum()
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
        return make_error(peek().position(), "expected '{' after 'enum'");
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
            return ChainableResult<EnumDeclaration>{member_result};
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
        return make_error(peek().position(), "expected '}' to close enum");
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

ChainableResult<EnumMember> Parser::parse_enum_member(std::int64_t &counter)
{
    // Expect identifier
    if (!check(TokenType::identifier)) {
        return make_error(peek().position(), "expected identifier for enum member");
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
            return make_error(
                member_pos,
                format_->format("expected expression after '=' for enum member '{}'", FormatParam{name, Style::bold}));
        }

        auto eval_result = evaluate_expression(expr_tokens);
        if (!eval_result.has_value()) {
            return ChainableResult<EnumMember>{
                make_error(
                    member_pos,
                    format_->format(
                        "failed to evaluate expression for enum member '{}'", FormatParam{name, Style::bold})),
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

ChainableResult<DefineStatement> Parser::parse_define()
{
    SourcePosition define_pos = peek().position();
    advance(); // consume 'define'

    // Skip whitespace (already handled by lexer, but newlines are significant)

    // Expect identifier for macro name
    if (!check(TokenType::identifier)) {
        return make_error(peek().position(), "expected identifier after '#define'");
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
        return ChainableResult<DefineStatement>{
            make_error(
                define_pos,
                format_->format("failed to evaluate expression for #define '{}'", FormatParam{name, Style::bold})),
            result};
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

ChainableResult<std::int64_t> Parser::evaluate_expression(const std::vector<Token> &expr_tokens)
{
    if (expr_tokens.empty()) {
        return make_error(SourcePosition{}, "empty expression");
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

ChainableResult<std::int64_t> Parser::evaluate_postfix(const std::vector<Token> &postfix)
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
                return make_error(
                    token.position(),
                    format_->format("unknown identifier '{}'", FormatParam{token.text(), Style::bold}));
            }
        }
        else if (is_operator(token.type())) {
            // Check if it's a unary operator (text starts with 'u')
            if (token.text().size() > 1 && token.text()[0] == 'u') {
                if (values.empty()) {
                    return make_error(
                        token.position(),
                        format_->format(
                            "unary operator '{}' missing operand", FormatParam{token.text().substr(1), Style::bold}));
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
                    return make_error(
                        token.position(),
                        format_->format("unknown unary operator '{}'", FormatParam{token.text(), Style::bold}));
                }
                values.push(result);
            }
            else {
                // Binary operator
                if (values.size() < 2) {
                    return make_error(
                        token.position(),
                        format_->format(
                            "binary operator '{}' missing operands", FormatParam{token.text(), Style::bold}));
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
                        return make_error(token.position(), "division by zero");
                    }
                    result = left / right;
                    break;
                case TokenType::percent:
                    if (right == 0) {
                        return make_error(token.position(), "modulo by zero");
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
                    return make_error(
                        token.position(),
                        format_->format("unknown binary operator '{}'", FormatParam{token.text(), Style::bold}));
                }
                values.push(result);
            }
        }
    }

    if (values.empty()) {
        return make_error(SourcePosition{}, "expression evaluated to no value");
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

ChainableResult<std::vector<ArrayDeclaration>> Parser::parse_pointer_arrays()
{
    std::vector<ArrayDeclaration> arrays;

    // Reset position to beginning
    current_ = 0;

    while (!is_at_end()) {
        // Skip newlines
        while (check(TokenType::newline)) {
            advance();
        }

        if (is_at_end()) {
            break;
        }

        // Look for pattern: [const] TYPE * [const] IDENTIFIER [] = { ... }
        // We need to find an identifier followed by [] = {
        // Start by looking for an identifier that could be an array name

        std::size_t scan_start = current_;

        // Skip 'const' if present
        if (check(TokenType::identifier) && peek().text() == "const") {
            advance();
        }

        // Skip type identifier (e.g., u16, int, etc.)
        if (check(TokenType::identifier)) {
            advance();
        }
        else {
            // Not a declaration, skip this token
            if (current_ == scan_start) {
                advance();
            }
            continue;
        }

        // Look for * (pointer)
        if (!check(TokenType::star)) {
            continue;
        }
        advance(); // consume *

        // Skip 'const' if present after *
        if (check(TokenType::identifier) && peek().text() == "const") {
            advance();
        }

        // Now we should have the array name identifier
        if (!check(TokenType::identifier)) {
            continue;
        }

        std::string array_name = peek().text();
        SourcePosition name_pos = peek().position();
        advance(); // consume identifier

        // Look for []
        if (!check(TokenType::left_bracket)) {
            continue;
        }
        advance(); // consume [

        if (!check(TokenType::right_bracket)) {
            continue;
        }
        advance(); // consume ]

        // Look for =
        if (!check(TokenType::equal)) {
            continue;
        }
        advance(); // consume =

        // Skip any newlines before {
        while (check(TokenType::newline)) {
            advance();
        }

        // Look for {
        if (!check(TokenType::left_brace)) {
            continue;
        }

        // Found a pointer array declaration - extract elements
        std::vector<Token> brace_contents = collect_brace_contents(tokens_, current_);
        std::vector<std::string> elements = extract_identifier_elements(brace_contents);

        // Skip past the closing brace
        current_ = skip_balanced_braces(tokens_, current_);

        // Skip optional semicolon
        if (check(TokenType::semicolon)) {
            advance();
        }

        arrays.emplace_back(std::move(array_name), std::move(elements), name_pos);
    }

    return arrays;
}

ChainableResult<std::vector<FunctionDefinition>> Parser::parse_functions(const std::optional<std::string> &name_prefix)
{
    std::vector<FunctionDefinition> functions;

    // Reset position to beginning
    current_ = 0;

    while (!is_at_end()) {
        // Skip newlines
        while (check(TokenType::newline)) {
            advance();
        }

        if (is_at_end()) {
            break;
        }

        // Look for pattern: [static] TYPE IDENTIFIER ( params ) { body }
        std::size_t scan_start = current_;

        // Skip 'static' if present
        if (check(TokenType::identifier) && peek().text() == "static") {
            advance();
        }

        // Skip return type identifier (e.g., void, int, etc.)
        if (check(TokenType::identifier)) {
            advance();
        }
        else {
            // Not a function, skip this token
            if (current_ == scan_start) {
                advance();
            }
            continue;
        }

        // Now we should have the function name identifier
        if (!check(TokenType::identifier)) {
            continue;
        }

        std::string func_name = peek().text();
        SourcePosition name_pos = peek().position();
        advance(); // consume identifier

        // Look for (
        if (!check(TokenType::left_paren)) {
            continue;
        }

        // Skip past the parameter list
        current_ = skip_balanced_parens(tokens_, current_);

        // Skip any newlines before {
        while (check(TokenType::newline)) {
            advance();
        }

        // Look for {
        if (!check(TokenType::left_brace)) {
            continue;
        }

        // Found a function definition - extract body tokens
        std::vector<Token> body_tokens = collect_brace_contents(tokens_, current_);

        // Skip past the closing brace
        current_ = skip_balanced_braces(tokens_, current_);

        // Apply name prefix filter if provided
        if (name_prefix.has_value()) {
            if (!func_name.starts_with(name_prefix.value())) {
                continue;
            }
        }

        functions.emplace_back(std::move(func_name), std::move(body_tokens), name_pos);
    }

    return functions;
}

} // namespace porytiles2
