#pragma once

#include <string>
#include <vector>

#include "porytiles/utilities/c_parser/source_position.hpp"
#include "porytiles/utilities/c_parser/token.hpp"

namespace porytiles {

/// @brief Represents a parsed C function definition.
///
/// @details
/// FunctionDefinition captures the name and body tokens of a C function definition like:
/// @code
/// static void QueueAnimTiles_General_Flower(u16 timer) {
///     u16 i = timer % ARRAY_COUNT(gTilesetAnims_General_Flower);
///     AppendTilesetAnimToBuffer(..., TILE_OFFSET_4BPP(12), 4 * TILE_SIZE_4BPP);
/// }
/// @endcode
///
/// Rather than parsing the function body into an AST, we capture the raw tokens between the braces. This allows for
/// targeted pattern matching within the body (e.g., finding TILE_OFFSET_4BPP calls) without implementing a full C
/// parser.
///
/// @invariant name_ is never empty
/// @invariant position_ contains valid 1-based line and column numbers
class FunctionDefinition {
  public:
    /// @brief Constructs a FunctionDefinition.
    ///
    /// @param name The function name
    /// @param body_tokens All tokens between the opening and closing braces (exclusive of braces)
    /// @param position The source position of the function name
    FunctionDefinition(std::string name, std::vector<Token> body_tokens, SourcePosition position)
        : name_{std::move(name)}, body_tokens_{std::move(body_tokens)}, position_{position}
    {
    }

    /// @brief Returns the function name.
    ///
    /// @return A const reference to the name
    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

    /// @brief Returns the function body tokens.
    ///
    /// @details
    /// Returns all tokens that appeared between the opening and closing braces of the function body. The braces
    /// themselves are not included. This allows callers to perform targeted pattern matching for specific constructs
    /// like macro invocations.
    ///
    /// @return A const reference to the body token vector
    [[nodiscard]] const std::vector<Token> &body_tokens() const
    {
        return body_tokens_;
    }

    /// @brief Returns the source position of the function name.
    ///
    /// @return A const reference to the source position
    [[nodiscard]] const SourcePosition &position() const
    {
        return position_;
    }

  private:
    std::string name_;
    std::vector<Token> body_tokens_;
    SourcePosition position_;
};

} // namespace porytiles
