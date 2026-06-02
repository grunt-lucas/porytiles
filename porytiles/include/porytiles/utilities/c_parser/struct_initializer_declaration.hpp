#pragma once

#include <optional>
#include <string>
#include <vector>

#include "porytiles/utilities/c_parser/designated_initializer_field.hpp"
#include "porytiles/utilities/c_parser/source_position.hpp"

namespace porytiles {

/**
 * @brief Represents a parsed C struct variable declaration with its initializer fields.
 *
 * @details
 * StructInitializerDeclaration captures the full structure of C struct variable declarations
 * including the designated initializer fields:
 * @code
 * const struct Tileset gTileset_General = {
 *     .isCompressed = TRUE,
 *     .isSecondary = FALSE,
 *     .tiles = gTilesetTiles_General,
 *     .palettes = gTilesetPalettes_General,
 *     .metatiles = gMetatiles_General,
 *     .metatileAttributes = gMetatileAttributes_General,
 *     .callback = InitTilesetAnim_General,
 * };
 * @endcode
 *
 * This extends StructVariableDeclaration by also parsing and storing the initializer fields,
 * enabling extraction of tileset metadata like isSecondary and variable references.
 *
 * @invariant struct_type_ is never empty
 * @invariant variable_name_ is never empty
 * @invariant position_ contains valid 1-based line and column numbers
 */
class StructInitializerDeclaration {
  public:
    /**
     * @brief Constructs a StructInitializerDeclaration.
     *
     * @param struct_type The struct type name (e.g., "Tileset")
     * @param variable_name The variable name (e.g., "gTileset_General")
     * @param fields The designated initializer fields
     * @param position The source position of the variable name
     */
    StructInitializerDeclaration(
        std::string struct_type,
        std::string variable_name,
        std::vector<DesignatedInitializerField> fields,
        SourcePosition position)
        : struct_type_{std::move(struct_type)}, variable_name_{std::move(variable_name)}, fields_{std::move(fields)},
          position_{position}
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
     * @brief Returns all designated initializer fields.
     *
     * @return A const reference to the vector of fields
     */
    [[nodiscard]] const std::vector<DesignatedInitializerField> &fields() const
    {
        return fields_;
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

    /**
     * @brief Looks up the value of a field by name.
     *
     * @param field_name The name of the field to look up
     * @return The value if the field exists, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<std::string> field_value(const std::string &field_name) const
    {
        for (const auto &field : fields_) {
            if (field.field_name() == field_name) {
                return field.value();
            }
        }
        return std::nullopt;
    }

  private:
    std::string struct_type_;
    std::string variable_name_;
    std::vector<DesignatedInitializerField> fields_;
    SourcePosition position_;
};

} // namespace porytiles
