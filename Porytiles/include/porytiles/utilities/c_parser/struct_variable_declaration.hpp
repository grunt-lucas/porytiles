#pragma once

#include <string>

#include "porytiles/utilities/c_parser/source_position.hpp"

namespace porytiles {

/**
 * @brief Represents a parsed C struct variable declaration.
 *
 * @details
 * StructVariableDeclaration captures the structure of C struct variable declarations like:
 * @code
 * const struct Tileset gTileset_General = {
 *     .isCompressed = TRUE,
 *     .isSecondary = FALSE,
 *     .tiles = gTilesetTiles_General,
 *     ...
 * };
 * @endcode
 *
 * The parser extracts the struct type name and variable name. The initializer body contents are not captured as only
 * the variable name is needed for tileset discovery. This is used by ProjectTilesetMetadataProvider to extract tileset
 * names from headers.h files.
 *
 * @invariant struct_type_ is never empty
 * @invariant variable_name_ is never empty
 * @invariant position_ contains valid 1-based line and column numbers
 */
class StructVariableDeclaration {
  public:
    /**
     * @brief Constructs a StructVariableDeclaration.
     *
     * @param struct_type The struct type name (e.g., "Tileset")
     * @param variable_name The variable name (e.g., "gTileset_General")
     * @param position The source position of the variable name
     */
    StructVariableDeclaration(std::string struct_type, std::string variable_name, SourcePosition position)
        : struct_type_{std::move(struct_type)}, variable_name_{std::move(variable_name)}, position_{position}
    {
    }

    /**
     * @brief Returns the struct type name.
     *
     * @return A const reference to the struct type name (e.g., "Tileset")
     */
    [[nodiscard]] const std::string &struct_type() const
    {
        return struct_type_;
    }

    /**
     * @brief Returns the variable name.
     *
     * @return A const reference to the variable name (e.g., "gTileset_General")
     */
    [[nodiscard]] const std::string &variable_name() const
    {
        return variable_name_;
    }

    /**
     * @brief Returns the source position of the variable name.
     *
     * @return A const reference to the source position
     */
    [[nodiscard]] const SourcePosition &position() const
    {
        return position_;
    }

  private:
    std::string struct_type_;
    std::string variable_name_;
    SourcePosition position_;
};

} // namespace porytiles
