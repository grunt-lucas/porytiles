#pragma once

#include <string>
#include <vector>

#include "porytiles/utilities/c_parser/token.hpp"

namespace porytiles {

/**
 * @brief Represents a parsed function call expression within a token stream.
 *
 * @details
 * FunctionCallInfo captures the function name and arguments from a call expression like:
 * @code
 * AppendTilesetAnimToBuffer(gTilesetAnims_General_Flower[i], TILE_OFFSET_4BPP(508), 4 * TILE_SIZE_4BPP)
 * @endcode
 *
 * Arguments are stored as raw token vectors, allowing callers to perform pattern matching on each argument
 * (e.g., extracting the integer from `TILE_OFFSET_4BPP(X)`).
 *
 * @invariant function_name_ is never empty
 */
class FunctionCallInfo {
  public:
    /**
     * @brief Constructs a FunctionCallInfo.
     *
     * @param function_name The name of the called function
     * @param arguments Vector of argument token sequences (one per argument)
     * @param start_index The token index where this call begins in the source token stream
     */
    FunctionCallInfo(std::string function_name, std::vector<std::vector<Token>> arguments, std::size_t start_index)
        : function_name_{std::move(function_name)}, arguments_{std::move(arguments)}, start_index_{start_index}
    {
    }

    /**
     * @brief Returns the function name.
     *
     * @return A const reference to the function name
     */
    [[nodiscard]] const std::string &function_name() const
    {
        return function_name_;
    }

    /**
     * @brief Returns the arguments as token sequences.
     *
     * @details
     * Each element in the vector represents one argument as a sequence of tokens. Arguments are separated by commas
     * in the source, but the comma tokens are not included in the argument vectors.
     *
     * @return A const reference to the arguments vector
     */
    [[nodiscard]] const std::vector<std::vector<Token>> &arguments() const
    {
        return arguments_;
    }

    /**
     * @brief Returns the number of arguments.
     *
     * @return The argument count
     */
    [[nodiscard]] std::size_t argument_count() const
    {
        return arguments_.size();
    }

    /**
     * @brief Returns the argument at the specified index.
     *
     * @param index The argument index (0-based)
     * @pre index must be less than argument_count()
     * @return A const reference to the argument token sequence
     */
    [[nodiscard]] const std::vector<Token> &argument_at(std::size_t index) const
    {
        return arguments_.at(index);
    }

    /**
     * @brief Returns the token index where this call begins.
     *
     * @return The start index in the source token stream
     */
    [[nodiscard]] std::size_t start_index() const
    {
        return start_index_;
    }

    /**
     * @brief Reconstructs a human-readable call expression from this function call.
     *
     * @details
     * Joins all argument tokens with commas to produce a string like:
     * @c AppendTilesetAnimToBuffer(gTilesetAnims_General_Flower[i], TILE_OFFSET_4BPP(508), 4 * TILE_SIZE_4BPP)
     *
     * @return A reconstructed call expression string
     */
    [[nodiscard]] std::string reconstruct_call_text() const
    {
        std::string result = function_name_ + "(";
        for (std::size_t arg_idx = 0; arg_idx < arguments_.size(); ++arg_idx) {
            if (arg_idx > 0) {
                result += ", ";
            }
            const auto &arg_tokens = arguments_[arg_idx];
            for (std::size_t tok_idx = 0; tok_idx < arg_tokens.size(); ++tok_idx) {
                if (tok_idx > 0) {
                    bool suppress_space =
                        arg_tokens[tok_idx - 1].is_any_of(TokenType::left_paren, TokenType::left_bracket) ||
                        arg_tokens[tok_idx].is_any_of(TokenType::right_paren, TokenType::right_bracket);
                    if (!suppress_space) {
                        result += " ";
                    }
                }
                result += arg_tokens[tok_idx].text();
            }
        }
        result += ")";
        return result;
    }

  private:
    std::string function_name_;
    std::vector<std::vector<Token>> arguments_;
    std::size_t start_index_;
};

/**
 * @brief Finds all calls to a specific function within a token stream.
 *
 * @details
 * Searches for patterns of the form `identifier(target_name) + lparen + ... + rparen` and extracts the function call
 * information including arguments. Arguments are split by comma tokens at the top level (parentheses are tracked
 * to handle nested calls).
 *
 * This is useful for finding specific macro or function calls within a function body, e.g., finding all
 * `AppendTilesetAnimToBuffer(...)` calls within a queue function.
 *
 * @param tokens The token stream to search
 * @param target_function_name The function name to search for
 * @return Vector of FunctionCallInfo for all matching calls
 */
[[nodiscard]] std::vector<FunctionCallInfo>
find_function_calls(const std::vector<Token> &tokens, const std::string &target_function_name);

/**
 * @brief Finds all function call expressions within a token stream.
 *
 * @details
 * Unlike find_function_calls(), this returns ALL function calls regardless of name. This is useful for discovering
 * what functions are called within a code block.
 *
 * @param tokens The token stream to search
 * @return Vector of FunctionCallInfo for all function calls found
 */
[[nodiscard]] std::vector<FunctionCallInfo> find_all_function_calls(const std::vector<Token> &tokens);

} // namespace porytiles
