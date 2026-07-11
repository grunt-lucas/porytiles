#include "porytiles/utilities/c_parser/parser.hpp"

#include <stack>

#include "porytiles/utilities/c_parser/c_parser_context.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

namespace {

/// @brief Skips tokens until finding a matching closing brace, handling nested braces.
///
/// @param tokens The token vector
/// @param start Position of the opening brace
/// @return Position after the closing brace, or tokens.size() if not found
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

/// @brief Collects all tokens between braces, handling nested braces.
///
/// @param tokens The token vector
/// @param start Position of the opening brace
/// @return Vector of tokens between the braces (exclusive of braces themselves)
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

/// @brief Extracts identifier names from a comma-separated list in brace contents.
///
/// @param brace_contents Tokens from inside the braces
/// @return Vector of identifier names in order
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

/// @brief Skips tokens until finding a matching closing parenthesis.
///
/// @param tokens The token vector
/// @param start Position of the opening parenthesis
/// @return Position after the closing parenthesis, or tokens.size() if not found
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

/// @brief Determines whether a token names a graphic-inclusion macro understood by the INCBIN parser.
///
/// @details
/// Recognizes the plain @c INCBIN_* family (e.g. @c INCBIN_U32) as well as the alternate form
/// auto-conversion @c INCGFX_* family (e.g. @c INCGFX_U32). Both forms take the source/binary path as their
/// first string-literal argument, so the parser handles them identically.
///
/// @param token The macro identifier text to test.
/// @return @c true if @p token begins with @c "INCBIN_" or @c "INCGFX_".
[[nodiscard]] bool is_gfx_inclusion_macro(const std::string &token)
{
    return token.starts_with("INCBIN_") || token.starts_with("INCGFX_");
}

} // namespace

FormattableError Parser::make_error(SourcePosition pos, std::string message) const
{
    if (context_ != nullptr) {
        return context_->make_error(pos, std::move(message));
    }
    return FormattableError{format_->format("{}:{}: {}", pos.line, pos.column, message)};
}

Parser::CondState Parser::effective_cond_state() const
{
    bool any_both = false;
    for (const auto &frame : cond_stack_) {
        if (frame.state == CondState::skipping) {
            return CondState::skipping;
        }
        if (frame.state == CondState::both) {
            any_both = true;
        }
    }
    return any_both ? CondState::both : CondState::active;
}

std::pair<Parser::CondState, bool> Parser::classify_ifdef(bool negated)
{
    // Precondition: the parser is positioned just after '#ifdef'/'#ifndef'. Read the macro name and decide.
    std::string name;
    if (check(TokenType::identifier)) {
        name = peek().text();
    }
    skip_to_next_line();

    if (name.empty() || !defined_names_.contains(name)) {
        // We cannot prove the macro is undefined (it may come from an unparsed include), so stay undecidable.
        return {CondState::both, false};
    }
    const bool condition = negated ? false : true; // name is known-defined; #ifdef is true, #ifndef is false
    return {condition ? CondState::active : CondState::skipping, true};
}

std::optional<std::vector<Token>> Parser::substitute_defined_operators(const std::vector<Token> &expr) const
{
    std::vector<Token> out;
    std::size_t i = 0;
    while (i < expr.size()) {
        if (!expr[i].is(TokenType::kw_defined)) {
            out.push_back(expr[i]);
            ++i;
            continue;
        }

        // Accept either `defined(NAME)` or `defined NAME`.
        std::string name;
        SourcePosition pos = expr[i].position();
        std::size_t next = i + 1;
        if (next < expr.size() && expr[next].is(TokenType::left_paren)) {
            if (next + 1 < expr.size() && expr[next + 1].is(TokenType::identifier)) {
                name = expr[next + 1].text();
            }
            std::size_t close = next + 1;
            while (close < expr.size() && !expr[close].is(TokenType::right_paren)) {
                ++close;
            }
            i = (close < expr.size()) ? close + 1 : expr.size();
        }
        else if (next < expr.size() && expr[next].is(TokenType::identifier)) {
            name = expr[next].text();
            i = next + 1;
        }
        else {
            return std::nullopt; // malformed defined operator
        }

        if (name.empty() || !defined_names_.contains(name)) {
            // The name is not known-defined, so the whole condition is undecidable.
            return std::nullopt;
        }
        out.push_back(Token{"1", static_cast<std::int64_t>(1), pos});
    }
    return out;
}

std::pair<Parser::CondState, bool> Parser::classify_if_expression()
{
    // Precondition: the parser is positioned just after '#if'/'#elif'. Collect and evaluate the condition tokens.
    std::vector<Token> expr = collect_expression_tokens();

    auto substituted = substitute_defined_operators(expr);
    if (!substituted.has_value() || substituted->empty()) {
        return {CondState::both, false};
    }

    auto eval = evaluate_expression(substituted.value());
    if (!eval.has_value()) {
        // An unresolved identifier or other evaluation failure leaves the condition undecidable.
        return {CondState::both, false};
    }
    return {eval.value() != 0 ? CondState::active : CondState::skipping, true};
}

bool Parser::try_handle_conditional()
{
    // Precondition: the current token is '#'.
    const TokenType directive = peek_next().type();

    switch (directive) {
    case TokenType::kw_ifdef:
    case TokenType::kw_ifndef: {
        const bool negated = directive == TokenType::kw_ifndef;
        advance(); // '#'
        advance(); // ifdef / ifndef
        auto [state, decidable] = classify_ifdef(negated);
        cond_stack_.push_back(ConditionalFrame{state, decidable, state == CondState::active});
        return true;
    }
    case TokenType::kw_if: {
        advance(); // '#'
        advance(); // if
        auto [state, decidable] = classify_if_expression();
        cond_stack_.push_back(ConditionalFrame{state, decidable, state == CondState::active});
        return true;
    }
    case TokenType::kw_elif: {
        advance(); // '#'
        advance(); // elif
        auto [state, decidable] = classify_if_expression();
        if (!cond_stack_.empty()) {
            ConditionalFrame &frame = cond_stack_.back();
            if (!frame.decidable || !decidable) {
                // Any undecidable link pins the whole chain to both.
                frame.state = CondState::both;
                frame.decidable = false;
            }
            else if (frame.branch_taken) {
                frame.state = CondState::skipping;
            }
            else {
                frame.state = state;
                frame.branch_taken = state == CondState::active;
            }
        }
        return true;
    }
    case TokenType::kw_else: {
        advance(); // '#'
        advance(); // else
        skip_to_next_line();
        if (!cond_stack_.empty()) {
            ConditionalFrame &frame = cond_stack_.back();
            if (frame.decidable) {
                frame.state = frame.branch_taken ? CondState::skipping : CondState::active;
                frame.branch_taken = true;
            }
            // Undecidable chains stay both.
        }
        return true;
    }
    case TokenType::kw_endif: {
        advance(); // '#'
        advance(); // endif
        skip_to_next_line();
        if (!cond_stack_.empty()) {
            cond_stack_.pop_back();
        }
        return true;
    }
    default:
        return false; // not a conditional directive
    }
}

ChainableResult<std::vector<DefineStatement>> Parser::parse_defines()
{
    std::vector<DefineStatement> defines;
    cond_stack_.clear();

    while (!is_at_end()) {
        // Look for # token
        if (check(TokenType::hash)) {
            if (try_handle_conditional()) {
                continue;
            }
            advance(); // consume #

            // Check if this is a #define
            if (check(TokenType::kw_define)) {
                auto result = parse_define();
                if (!result.has_value()) {
                    return ChainableResult<std::vector<DefineStatement>>{result};
                }
                record_define(std::move(result).value(), defines);
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

void Parser::record_define(DefineStatement statement, std::vector<DefineStatement> &out)
{
    const CondState eff = effective_cond_state();
    if (eff == CondState::skipping) {
        // The define lives in a region we can prove is inactive, so it neither defines a name nor contributes a value.
        return;
    }

    if (statement.has_int_value()) {
        auto it = defined_values_.find(statement.name());
        if (eff == CondState::both && it != defined_values_.end() && it->second != statement.int_value()) {
            ambiguous_defines_.insert(statement.name());
            scan_warnings_.push_back(format_->format(
                "conflicting redefinition of '{}' inside an undecidable conditional; using the last value",
                FormatParam{statement.name(), Style::bold}));
        }
        defined_values_[statement.name()] = statement.int_value();
    }
    defined_names_.insert(statement.name());
    out.push_back(std::move(statement));
}

Parser::DefineParseOutcome Parser::parse_define_tolerant()
{
    SourcePosition define_pos = peek().position();
    advance(); // consume 'define'

    if (!check(TokenType::identifier)) {
        skip_to_next_line();
        return {std::nullopt, SkippedConstruct{"", define_pos, "expected identifier after #define"}};
    }

    std::string name = peek().text();
    SourcePosition name_pos = peek().position();
    std::size_t name_end_column = name_pos.column + name.size();
    advance(); // consume identifier

    // Parametric (function-like) macro: '(' immediately follows the name.
    if (check(TokenType::left_paren)) {
        SourcePosition paren_pos = peek().position();
        if (paren_pos.line == name_pos.line && paren_pos.column == name_end_column) {
            skip_to_next_line();
            return {DefineStatement{std::move(name), define_pos}, std::nullopt};
        }
    }

    // Flag-like define with no value.
    if (is_at_line_end()) {
        if (check(TokenType::newline)) {
            advance();
        }
        return {DefineStatement{std::move(name), define_pos}, std::nullopt};
    }

    // String value.
    if (check(TokenType::string_literal)) {
        std::string value = peek().text();
        advance();
        skip_to_next_line();
        return {DefineStatement{std::move(name), std::move(value), define_pos}, std::nullopt};
    }

    // Expression value.
    std::vector<Token> expr_tokens = collect_expression_tokens();
    if (expr_tokens.empty()) {
        return {DefineStatement{std::move(name), define_pos}, std::nullopt};
    }

    auto result = evaluate_expression(expr_tokens);
    if (!result.has_value()) {
        return {std::nullopt, SkippedConstruct{std::move(name), define_pos, "unevaluable value expression"}};
    }
    return {DefineStatement{std::move(name), result.value(), define_pos}, std::nullopt};
}

TolerantDefineScan Parser::parse_defines_tolerant()
{
    TolerantDefineScan scan;
    current_ = 0;
    cond_stack_.clear();

    while (!is_at_end()) {
        if (check(TokenType::hash)) {
            if (try_handle_conditional()) {
                continue;
            }
            advance(); // consume #

            if (check(TokenType::kw_define)) {
                auto outcome = parse_define_tolerant();
                if (effective_cond_state() == CondState::skipping) {
                    // Provably inactive region: consume but record nothing.
                    continue;
                }
                if (outcome.statement.has_value()) {
                    record_define(std::move(outcome.statement).value(), scan.defines);
                }
                else if (outcome.skipped.has_value()) {
                    if (!outcome.skipped->name.empty()) {
                        defined_names_.insert(outcome.skipped->name);
                    }
                    scan.skipped.push_back(std::move(outcome.skipped).value());
                }
            }
            else {
                skip_to_next_line();
            }
        }
        else {
            advance();
        }
    }

    return scan;
}

TolerantEnumMember Parser::parse_enum_member_tolerant(std::int64_t &counter, bool &counter_valid)
{
    std::string name = peek().text();
    SourcePosition member_pos = peek().position();
    advance(); // consume identifier

    if (check(TokenType::equal)) {
        advance(); // consume '='
        std::vector<Token> expr_tokens = collect_enum_value_tokens();
        if (!expr_tokens.empty()) {
            auto eval = evaluate_expression(expr_tokens);
            if (eval.has_value()) {
                counter = eval.value();
                counter_valid = true;
                TolerantEnumMember member{std::move(name), counter, member_pos};
                counter++;
                defined_values_[member.name] = member.value.value();
                return member;
            }
        }
        // Unevaluable (or empty) explicit value poisons the counter.
        counter_valid = false;
        return TolerantEnumMember{std::move(name), std::nullopt, member_pos};
    }

    // Implicit member: usable only while the counter is trustworthy.
    std::optional<std::int64_t> value = counter_valid ? std::optional<std::int64_t>{counter} : std::nullopt;
    TolerantEnumMember member{std::move(name), value, member_pos};
    counter++;
    if (member.value.has_value()) {
        defined_values_[member.name] = member.value.value();
    }
    return member;
}

std::optional<TolerantEnum> Parser::parse_enum_tolerant()
{
    SourcePosition enum_pos = peek().position();
    advance(); // consume 'enum'

    std::optional<std::string> enum_name;
    if (check(TokenType::identifier)) {
        enum_name = peek().text();
        advance();
    }

    while (check(TokenType::newline)) {
        advance();
    }

    if (!check(TokenType::left_brace)) {
        // Not a definition we can parse (e.g. a forward use like `enum Foo bar;`); nothing to record.
        return std::nullopt;
    }
    advance(); // consume '{'

    std::vector<TolerantEnumMember> members;
    std::int64_t counter = 0;
    bool counter_valid = true;
    bool directive_seen = false;

    while (!is_at_end() && !check(TokenType::right_brace)) {
        while (check(TokenType::newline)) {
            advance();
        }
        if (check(TokenType::right_brace)) {
            break;
        }
        if (check(TokenType::hash)) {
            // A preprocessor directive inside the body. The scanner does not evaluate conditionals here, so every
            // member value beyond this point depends on a branch it cannot decide. Skip the directive line (rather
            // than lexing its tokens as phantom members) and record later members as valueless.
            skip_to_next_line();
            directive_seen = true;
            counter_valid = false;
            continue;
        }
        if (!check(TokenType::identifier)) {
            // Unexpected token inside the enum body; skip it to stay resilient.
            advance();
            continue;
        }

        members.push_back(parse_enum_member_tolerant(counter, counter_valid));
        if (directive_seen) {
            // Even an explicit '= value' cannot be trusted once a directive appeared: it may sit in an untaken branch.
            members.back().value = std::nullopt;
            counter_valid = false;
        }

        if (check(TokenType::comma)) {
            advance();
        }
        while (check(TokenType::newline)) {
            advance();
        }
    }

    if (check(TokenType::right_brace)) {
        advance(); // consume '}'
    }
    if (check(TokenType::semicolon)) {
        advance();
    }

    return TolerantEnum{std::move(enum_name), std::move(members), enum_pos};
}

TolerantEnumScan Parser::parse_enums_tolerant()
{
    TolerantEnumScan scan;
    current_ = 0;
    cond_stack_.clear();

    while (!is_at_end()) {
        if (check(TokenType::hash)) {
            if (try_handle_conditional()) {
                continue;
            }
            skip_to_next_line();
            continue;
        }
        if (check(TokenType::kw_enum)) {
            auto parsed = parse_enum_tolerant();
            if (parsed.has_value() && effective_cond_state() != CondState::skipping) {
                scan.enums.push_back(std::move(parsed).value());
            }
        }
        else {
            advance();
        }
    }

    return scan;
}

ChainableResult<std::vector<EnumDeclaration>> Parser::parse_enums()
{
    std::vector<EnumDeclaration> enums;

    // Reset position to beginning (allows calling both parse_defines and parse_enums)
    current_ = 0;
    cond_stack_.clear();

    while (!is_at_end()) {
        if (check(TokenType::hash)) {
            if (try_handle_conditional()) {
                continue;
            }
            // A non-conditional directive (e.g. #define, #include) cannot start an enum, so skip its line.
            skip_to_next_line();
            continue;
        }
        if (check(TokenType::kw_enum)) {
            auto result = parse_enum();
            if (!result.has_value()) {
                return ChainableResult<std::vector<EnumDeclaration>>{result};
            }
            if (effective_cond_state() != CondState::skipping) {
                enums.push_back(std::move(result).value());
            }
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

    // Skip newlines before opening brace (handles `enum\n{` style)
    while (check(TokenType::newline)) {
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
    defined_values_[member.name()] = member.int_value();
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

    // The symbol table and defined-name set are updated by record_define once the enclosing conditional state is known.
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

    // Reject any token the evaluator has no rule for (ternaries, casts, sizeof, etc). Dropping it and evaluating the
    // remaining tokens would produce a confidently wrong value; failing here degrades to "value unknown" instead.
    for (const Token &token : expr_tokens) {
        const bool supported = token.is(TokenType::integer_literal) || token.is(TokenType::identifier) ||
                               token.is(TokenType::left_paren) || token.is(TokenType::right_paren) ||
                               is_operator(token.type());
        if (!supported) {
            return make_error(
                token.position(),
                format_->format("unsupported token '{}' in expression", FormatParam{token.text(), Style::bold}));
        }
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
        // No other token kinds can appear: evaluate_expression rejects unsupported tokens before conversion.
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
                case TokenType::less:
                    result = left < right ? 1 : 0;
                    break;
                case TokenType::greater:
                    result = left > right ? 1 : 0;
                    break;
                case TokenType::less_equal:
                    result = left <= right ? 1 : 0;
                    break;
                case TokenType::greater_equal:
                    result = left >= right ? 1 : 0;
                    break;
                case TokenType::equal_equal:
                    result = left == right ? 1 : 0;
                    break;
                case TokenType::exclaim_equal:
                    result = left != right ? 1 : 0;
                    break;
                case TokenType::ampersand_ampersand:
                    result = (left != 0 && right != 0) ? 1 : 0;
                    break;
                case TokenType::pipe_pipe:
                    result = (left != 0 || right != 0) ? 1 : 0;
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
    if (values.size() != 1) {
        // Operands were left stranded, meaning the expression was not fully understood. Returning the top of the
        // stack here would be a confidently wrong answer.
        return make_error(SourcePosition{}, "expression did not reduce to a single value");
    }

    return values.top();
}

int Parser::operator_precedence(TokenType type) const
{
    // Lower number = higher precedence (evaluated first)
    // Based on C operator precedence
    switch (type) {
    case TokenType::tilde:
    case TokenType::exclaim:
        // Always unary; they must bind tighter than every binary operator so that ~5 & 3 means (~5) & 3. Unary minus
        // cannot be distinguished from binary minus here, but sharing precedence 4 evaluates it correctly anyway.
        return 2;
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

        // Look for pattern: [static] [const] TYPE * [const] IDENTIFIER [] = { ... }
        // We need to find an identifier followed by [] = {
        // Start by looking for an identifier that could be an array name

        std::size_t scan_start = current_;

        // Skip 'static' if present
        if (check(TokenType::identifier) && peek().text() == "static") {
            advance();
        }

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

ChainableResult<std::vector<FunctionDefinition>> Parser::parse_functions()
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

        functions.emplace_back(std::move(func_name), std::move(body_tokens), name_pos);
    }

    return functions;
}

ChainableResult<std::vector<StructVariableDeclaration>> Parser::parse_struct_variables()
{
    std::vector<StructVariableDeclaration> structs;

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

        // Look for pattern: [const] struct TYPE IDENTIFIER = { ... } [;]
        std::size_t scan_start = current_;

        // Skip 'const' if present
        if (check(TokenType::identifier) && peek().text() == "const") {
            advance();
        }

        // Look for 'struct' keyword (lexer treats it as an identifier)
        if (!check(TokenType::identifier) || peek().text() != "struct") {
            // Not a struct declaration, skip this token
            if (current_ == scan_start) {
                advance();
            }
            continue;
        }
        advance(); // consume 'struct'

        // Get struct type name
        if (!check(TokenType::identifier)) {
            continue;
        }
        std::string struct_type = peek().text();
        advance(); // consume type name

        // Get variable name
        if (!check(TokenType::identifier)) {
            continue;
        }
        std::string variable_name = peek().text();
        SourcePosition name_pos = peek().position();
        advance(); // consume variable name

        // Look for '='
        if (!check(TokenType::equal)) {
            continue;
        }
        advance(); // consume '='

        // Skip any newlines before '{'
        while (check(TokenType::newline)) {
            advance();
        }

        // Look for '{'
        if (!check(TokenType::left_brace)) {
            continue;
        }

        // Skip balanced braces (we don't need the body contents)
        current_ = skip_balanced_braces(tokens_, current_);

        // Skip optional semicolon
        if (check(TokenType::semicolon)) {
            advance();
        }

        structs.emplace_back(std::move(struct_type), std::move(variable_name), name_pos);
    }

    return structs;
}

ChainableResult<std::vector<StructInitializerDeclaration>> Parser::parse_struct_initializers()
{
    std::vector<StructInitializerDeclaration> structs;

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

        // Look for pattern: [const] struct TYPE IDENTIFIER = { .field = value, ... } [;]
        std::size_t scan_start = current_;

        // Skip 'const' if present
        if (check(TokenType::identifier) && peek().text() == "const") {
            advance();
        }

        // Look for 'struct' keyword (lexer treats it as an identifier)
        if (!check(TokenType::identifier) || peek().text() != "struct") {
            // Not a struct declaration, skip this token
            if (current_ == scan_start) {
                advance();
            }
            continue;
        }
        advance(); // consume 'struct'

        // Get struct type name
        if (!check(TokenType::identifier)) {
            continue;
        }
        std::string struct_type = peek().text();
        advance(); // consume type name

        // Get variable name
        if (!check(TokenType::identifier)) {
            continue;
        }
        std::string variable_name = peek().text();
        SourcePosition name_pos = peek().position();
        advance(); // consume variable name

        // Look for '='
        if (!check(TokenType::equal)) {
            continue;
        }
        advance(); // consume '='

        // Skip any newlines before '{'
        while (check(TokenType::newline)) {
            advance();
        }

        // Look for '{'
        if (!check(TokenType::left_brace)) {
            continue;
        }

        // Parse the designated initializer fields
        std::vector<DesignatedInitializerField> fields;
        std::vector<Token> brace_contents = collect_brace_contents(tokens_, current_);

        // Parse each .field = value pair from the brace contents
        std::size_t brace_pos = 0;
        while (brace_pos < brace_contents.size()) {
            // Skip newlines and commas
            while (brace_pos < brace_contents.size() && (brace_contents[brace_pos].is(TokenType::newline) ||
                                                         brace_contents[brace_pos].is(TokenType::comma))) {
                ++brace_pos;
            }

            if (brace_pos >= brace_contents.size()) {
                break;
            }

            // Look for '.'
            if (!brace_contents[brace_pos].is(TokenType::period)) {
                // Skip until next comma or end
                while (brace_pos < brace_contents.size() && !brace_contents[brace_pos].is(TokenType::comma)) {
                    ++brace_pos;
                }
                continue;
            }
            ++brace_pos; // consume '.'

            // Get field name
            if (brace_pos >= brace_contents.size() || !brace_contents[brace_pos].is(TokenType::identifier)) {
                continue;
            }
            std::string field_name = brace_contents[brace_pos].text();
            SourcePosition field_pos = brace_contents[brace_pos].position();
            ++brace_pos; // consume field name

            // Look for '='
            if (brace_pos >= brace_contents.size() || !brace_contents[brace_pos].is(TokenType::equal)) {
                continue;
            }
            ++brace_pos; // consume '='

            // Skip newlines after '='
            while (brace_pos < brace_contents.size() && brace_contents[brace_pos].is(TokenType::newline)) {
                ++brace_pos;
            }

            // Get the value (identifier, or skip complex expressions)
            if (brace_pos >= brace_contents.size()) {
                continue;
            }

            std::string value;
            if (brace_contents[brace_pos].is(TokenType::identifier)) {
                value = brace_contents[brace_pos].text();
                ++brace_pos;
            }
            else if (brace_contents[brace_pos].is(TokenType::integer_literal)) {
                value = brace_contents[brace_pos].text();
                ++brace_pos;
            }
            else {
                // Skip complex expressions (nested braces, etc.) until comma or end
                while (brace_pos < brace_contents.size() && !brace_contents[brace_pos].is(TokenType::comma) &&
                       !brace_contents[brace_pos].is(TokenType::newline)) {
                    ++brace_pos;
                }
                continue;
            }

            fields.emplace_back(std::move(field_name), std::move(value), field_pos);
        }

        // Skip past the closing brace
        current_ = skip_balanced_braces(tokens_, current_);

        // Skip optional semicolon
        if (check(TokenType::semicolon)) {
            advance();
        }

        structs.emplace_back(std::move(struct_type), std::move(variable_name), std::move(fields), name_pos);
    }

    return structs;
}

ChainableResult<std::vector<IncbinDeclaration>> Parser::parse_incbin_arrays()
{
    std::vector<IncbinDeclaration> incbins;

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

        // Look for pattern: [static] [const] TYPE IDENTIFIER [] = INCBIN_MACRO("path");
        // or: [static] [const] TYPE IDENTIFIER [][SIZE] = { INCBIN_MACRO("p1"), ... };
        std::size_t scan_start = current_;

        // Skip 'static' if present
        if (check(TokenType::identifier) && peek().text() == "static") {
            advance();
        }

        // Skip 'const' if present
        if (check(TokenType::identifier) && peek().text() == "const") {
            advance();
        }

        // Get type (e.g., u32, u16)
        if (!check(TokenType::identifier)) {
            if (current_ == scan_start) {
                advance();
            }
            continue;
        }
        advance(); // consume type

        // Skip ALIGNED(N) directive if present (e.g., "const u16 ALIGNED(4) gTilesetPalettes_General")
        if (check(TokenType::identifier) && peek().text() == "ALIGNED") {
            advance(); // consume ALIGNED
            if (check(TokenType::left_paren)) {
                advance(); // consume '('
                // Skip until ')'
                while (!is_at_end() && !check(TokenType::right_paren)) {
                    advance();
                }
                if (check(TokenType::right_paren)) {
                    advance(); // consume ')'
                }
            }
        }

        // Get variable name
        if (!check(TokenType::identifier)) {
            continue;
        }
        std::string variable_name = peek().text();
        SourcePosition name_pos = peek().position();
        advance(); // consume variable name

        // Look for '['
        if (!check(TokenType::left_bracket)) {
            continue;
        }
        advance(); // consume '['

        // Look for ']'
        if (!check(TokenType::right_bracket)) {
            // Skip to end of line
            while (!is_at_end() && !check(TokenType::newline) && !check(TokenType::semicolon)) {
                advance();
            }
            continue;
        }
        advance(); // consume ']'

        // Check for optional second dimension [][SIZE]
        bool is_multi_dimensional = false;
        if (check(TokenType::left_bracket)) {
            is_multi_dimensional = true;
            advance(); // consume '['
            // Skip until ']'
            while (!is_at_end() && !check(TokenType::right_bracket)) {
                advance();
            }
            if (check(TokenType::right_bracket)) {
                advance(); // consume ']'
            }
        }

        // Look for '='
        if (!check(TokenType::equal)) {
            continue;
        }
        advance(); // consume '='

        // Skip any newlines after '='
        while (check(TokenType::newline)) {
            advance();
        }

        if (is_multi_dimensional) {
            // Multi-path: expect { INCBIN_MACRO("p1"), INCBIN_MACRO("p2"), ... }
            if (!check(TokenType::left_brace)) {
                continue;
            }

            std::vector<Token> brace_contents = collect_brace_contents(tokens_, current_);
            current_ = skip_balanced_braces(tokens_, current_);

            std::vector<std::string> paths;
            std::string macro_name;
            std::size_t brace_pos = 0;

            while (brace_pos < brace_contents.size()) {
                // Skip newlines and commas
                while (brace_pos < brace_contents.size() && (brace_contents[brace_pos].is(TokenType::newline) ||
                                                             brace_contents[brace_pos].is(TokenType::comma))) {
                    ++brace_pos;
                }

                if (brace_pos >= brace_contents.size()) {
                    break;
                }

                // Look for INCBIN_* identifier
                if (!brace_contents[brace_pos].is(TokenType::identifier)) {
                    ++brace_pos;
                    continue;
                }

                std::string token_text = brace_contents[brace_pos].text();
                if (!is_gfx_inclusion_macro(token_text)) {
                    ++brace_pos;
                    continue;
                }

                if (macro_name.empty()) {
                    macro_name = token_text;
                }
                ++brace_pos; // consume INCBIN_*

                // Look for '('
                if (brace_pos >= brace_contents.size() || !brace_contents[brace_pos].is(TokenType::left_paren)) {
                    continue;
                }
                ++brace_pos; // consume '('

                // Look for string literal
                if (brace_pos >= brace_contents.size() || !brace_contents[brace_pos].is(TokenType::string_literal)) {
                    continue;
                }
                paths.push_back(brace_contents[brace_pos].text());
                ++brace_pos; // consume string literal

                // Skip until ')' (may have other stuff)
                while (brace_pos < brace_contents.size() && !brace_contents[brace_pos].is(TokenType::right_paren)) {
                    ++brace_pos;
                }
                if (brace_pos < brace_contents.size()) {
                    ++brace_pos; // consume ')'
                }
            }

            if (!paths.empty()) {
                incbins.emplace_back(std::move(variable_name), std::move(macro_name), std::move(paths), name_pos);
            }
        }
        else {
            // Single path: expect INCBIN_MACRO("path")
            if (!check(TokenType::identifier)) {
                continue;
            }

            std::string token_text = peek().text();
            if (!is_gfx_inclusion_macro(token_text)) {
                continue;
            }
            std::string macro_name = token_text;
            advance(); // consume INCBIN_*

            // Look for '('
            if (!check(TokenType::left_paren)) {
                continue;
            }
            advance(); // consume '('

            // Look for string literal
            if (!check(TokenType::string_literal)) {
                continue;
            }
            std::string path = peek().text();
            advance(); // consume string literal

            // Skip until ')' and ';'
            while (!is_at_end() && !check(TokenType::semicolon) && !check(TokenType::newline)) {
                advance();
            }
            if (check(TokenType::semicolon)) {
                advance();
            }

            incbins.emplace_back(std::move(variable_name), std::move(macro_name), std::move(path), name_pos);
        }
    }

    return incbins;
}

std::vector<IndexedArrayEntry> Parser::parse_indexed_entries(const std::vector<Token> &brace_contents)
{
    std::vector<IndexedArrayEntry> entries;
    std::size_t pos = 0;

    while (pos < brace_contents.size()) {
        // Skip separators and blank lines between entries.
        while (pos < brace_contents.size() &&
               (brace_contents[pos].is(TokenType::newline) || brace_contents[pos].is(TokenType::comma))) {
            ++pos;
        }
        if (pos >= brace_contents.size()) {
            break;
        }

        // Each entry must start with '['. Anything else is skipped to the next comma to stay resilient.
        if (!brace_contents[pos].is(TokenType::left_bracket)) {
            while (pos < brace_contents.size() && !brace_contents[pos].is(TokenType::comma)) {
                ++pos;
            }
            continue;
        }
        ++pos; // consume '['

        // The designator: capture the first token's text (enum member name or numeric index).
        std::string index_name;
        SourcePosition entry_pos{};
        if (pos < brace_contents.size() &&
            (brace_contents[pos].is(TokenType::identifier) || brace_contents[pos].is(TokenType::integer_literal))) {
            index_name = brace_contents[pos].text();
            entry_pos = brace_contents[pos].position();
        }
        // Skip to the closing ']'.
        while (pos < brace_contents.size() && !brace_contents[pos].is(TokenType::right_bracket)) {
            ++pos;
        }
        if (pos < brace_contents.size()) {
            ++pos; // consume ']'
        }

        // Skip newlines before '='.
        while (pos < brace_contents.size() && brace_contents[pos].is(TokenType::newline)) {
            ++pos;
        }
        if (pos >= brace_contents.size() || !brace_contents[pos].is(TokenType::equal)) {
            // Malformed entry (no '='); skip to the next comma.
            while (pos < brace_contents.size() && !brace_contents[pos].is(TokenType::comma)) {
                ++pos;
            }
            continue;
        }
        ++pos; // consume '='

        // Collect value tokens up to the top-level comma that ends this entry.
        std::vector<Token> value_tokens;
        int depth = 0;
        while (pos < brace_contents.size()) {
            const Token &token = brace_contents[pos];
            if (depth == 0 && token.is(TokenType::comma)) {
                break;
            }
            if (token.is_any_of(TokenType::left_brace, TokenType::left_paren, TokenType::left_bracket)) {
                ++depth;
            }
            else if (token.is_any_of(TokenType::right_brace, TokenType::right_paren, TokenType::right_bracket)) {
                --depth;
            }
            if (!token.is(TokenType::newline)) {
                value_tokens.push_back(token);
            }
            ++pos;
        }

        std::optional<std::int64_t> value;
        if (!value_tokens.empty()) {
            auto eval = evaluate_expression(value_tokens);
            if (eval.has_value()) {
                value = eval.value();
            }
        }

        if (!index_name.empty()) {
            entries.push_back(IndexedArrayEntry{std::move(index_name), value, std::move(value_tokens), entry_pos});
        }
    }

    return entries;
}

ChainableResult<std::vector<IndexedArrayDeclaration>> Parser::parse_indexed_arrays()
{
    std::vector<IndexedArrayDeclaration> arrays;

    current_ = 0;
    cond_stack_.clear();

    while (!is_at_end()) {
        while (check(TokenType::newline)) {
            advance();
        }
        if (is_at_end()) {
            break;
        }

        if (check(TokenType::hash)) {
            if (try_handle_conditional()) {
                continue;
            }
            skip_to_next_line();
            continue;
        }

        // Look for: [static] [const] TYPE IDENTIFIER [SIZE_EXPR] = { [index] = value, ... };
        std::size_t scan_start = current_;

        if (check(TokenType::identifier) && peek().text() == "static") {
            advance();
        }
        if (check(TokenType::identifier) && peek().text() == "const") {
            advance();
        }

        // Type identifier.
        if (!check(TokenType::identifier)) {
            if (current_ == scan_start) {
                advance();
            }
            continue;
        }
        advance(); // consume type

        // Array name.
        if (!check(TokenType::identifier)) {
            continue;
        }
        std::string array_name = peek().text();
        SourcePosition name_pos = peek().position();
        advance(); // consume name

        // Size expression in brackets.
        if (!check(TokenType::left_bracket)) {
            continue;
        }
        advance(); // consume '['
        int bracket_depth = 1;
        while (!is_at_end() && bracket_depth > 0) {
            if (check(TokenType::left_bracket)) {
                ++bracket_depth;
            }
            else if (check(TokenType::right_bracket)) {
                --bracket_depth;
            }
            advance();
        }

        // Optional '=' then a brace initializer.
        while (check(TokenType::newline)) {
            advance();
        }
        if (!check(TokenType::equal)) {
            continue;
        }
        advance(); // consume '='
        while (check(TokenType::newline)) {
            advance();
        }
        if (!check(TokenType::left_brace)) {
            continue;
        }

        std::vector<Token> brace_contents = collect_brace_contents(tokens_, current_);
        current_ = skip_balanced_braces(tokens_, current_);
        if (check(TokenType::semicolon)) {
            advance();
        }

        std::vector<IndexedArrayEntry> entries = parse_indexed_entries(brace_contents);
        if (effective_cond_state() != CondState::skipping) {
            arrays.push_back(IndexedArrayDeclaration{std::move(array_name), std::move(entries), name_pos});
        }
    }

    return arrays;
}

} // namespace porytiles
