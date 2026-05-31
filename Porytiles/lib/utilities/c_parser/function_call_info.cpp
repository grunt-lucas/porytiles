#include "porytiles/utilities/c_parser/function_call_info.hpp"

#include <string>
#include <vector>

#include "porytiles/utilities/c_parser/token.hpp"

namespace porytiles {

namespace {

/**
 * @brief Parses function call arguments from a token stream starting after the opening parenthesis.
 *
 * @details
 * Extracts arguments by tracking parenthesis nesting and splitting on commas at the top level.
 *
 * @param tokens The token stream
 * @param start_index Index of the token AFTER the opening parenthesis
 * @param[out] end_index Set to the index of the closing parenthesis
 * @return Vector of argument token sequences
 */
[[nodiscard]] std::vector<std::vector<Token>>
parse_arguments(const std::vector<Token> &tokens, std::size_t start_index, std::size_t &end_index)
{
    std::vector<std::vector<Token>> arguments;
    std::vector<Token> current_arg;
    int paren_depth = 1;

    for (std::size_t i = start_index; i < tokens.size(); ++i) {
        const Token &tok = tokens[i];

        if (tok.is(TokenType::left_paren)) {
            ++paren_depth;
            current_arg.push_back(tok);
        }
        else if (tok.is(TokenType::right_paren)) {
            --paren_depth;
            if (paren_depth == 0) {
                // End of arguments
                if (!current_arg.empty()) {
                    arguments.push_back(std::move(current_arg));
                }
                end_index = i;
                return arguments;
            }
            current_arg.push_back(tok);
        }
        else if (tok.is(TokenType::comma) && paren_depth == 1) {
            // Top-level comma separates arguments
            arguments.push_back(std::move(current_arg));
            current_arg.clear();
        }
        else {
            current_arg.push_back(tok);
        }
    }

    // If we get here, we didn't find a matching close paren
    end_index = tokens.size();
    if (!current_arg.empty()) {
        arguments.push_back(std::move(current_arg));
    }
    return arguments;
}

} // namespace

std::vector<FunctionCallInfo>
find_function_calls(const std::vector<Token> &tokens, const std::string &target_function_name)
{
    std::vector<FunctionCallInfo> result;

    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        // Look for: identifier(target_name) + lparen
        if (tokens[i].is(TokenType::identifier) && tokens[i].text() == target_function_name &&
            tokens[i + 1].is(TokenType::left_paren)) {

            std::size_t end_index = 0;
            auto arguments = parse_arguments(tokens, i + 2, end_index);

            result.emplace_back(target_function_name, std::move(arguments), i);

            // Skip past the closing paren to avoid re-parsing nested calls
            i = end_index;
        }
    }

    return result;
}

std::vector<FunctionCallInfo> find_all_function_calls(const std::vector<Token> &tokens)
{
    std::vector<FunctionCallInfo> result;

    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        // Look for: identifier + lparen
        if (tokens[i].is(TokenType::identifier) && tokens[i + 1].is(TokenType::left_paren)) {
            std::string func_name = tokens[i].text();

            std::size_t end_index = 0;
            auto arguments = parse_arguments(tokens, i + 2, end_index);

            result.emplace_back(std::move(func_name), std::move(arguments), i);

            // Skip past the closing paren to avoid re-parsing nested calls
            i = end_index;
        }
    }

    return result;
}

} // namespace porytiles
