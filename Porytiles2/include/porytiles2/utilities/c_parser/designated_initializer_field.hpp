#pragma once

#include <string>

#include "porytiles2/utilities/c_parser/source_position.hpp"

namespace porytiles2 {

/**
 * @brief Represents a single designated initializer field from a C struct initialization.
 *
 * @details
 * DesignatedInitializerField captures a `.field = value` entry from C designated initializers like:
 * @code
 * const struct Tileset gTileset_General = {
 *     .isCompressed = TRUE,
 *     .isSecondary = FALSE,
 *     .tiles = gTilesetTiles_General,
 *     ...
 * };
 * @endcode
 *
 * Each field captures:
 * - The field name (e.g., "isSecondary")
 * - The value as a string (e.g., "FALSE", "gTilesetTiles_General", "NULL")
 * - The source position for error reporting
 *
 * @invariant field_name_ is never empty
 * @invariant value_ is never empty
 * @invariant position_ contains valid 1-based line and column numbers
 */
class DesignatedInitializerField {
  public:
    /**
     * @brief Constructs a DesignatedInitializerField.
     *
     * @param field_name The field name (e.g., "isSecondary")
     * @param value The value assigned to the field (e.g., "FALSE", "gTilesetTiles_General")
     * @param position The source position of the field name
     */
    DesignatedInitializerField(std::string field_name, std::string value, SourcePosition position)
        : field_name_{std::move(field_name)}, value_{std::move(value)}, position_{position}
    {
    }

    /**
     * @brief Returns the field name.
     *
     * @return A const reference to the field name (e.g., "isSecondary")
     */
    [[nodiscard]] const std::string &field_name() const
    {
        return field_name_;
    }

    /**
     * @brief Returns the field value.
     *
     * @return A const reference to the value (e.g., "FALSE", "gTilesetTiles_General")
     */
    [[nodiscard]] const std::string &value() const
    {
        return value_;
    }

    /**
     * @brief Returns the source position of the field name.
     *
     * @return A const reference to the source position
     */
    [[nodiscard]] const SourcePosition &position() const
    {
        return position_;
    }

  private:
    std::string field_name_;
    std::string value_;
    SourcePosition position_;
};

} // namespace porytiles2
