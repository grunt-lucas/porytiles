#pragma once

#include <string>
#include <vector>

#include "porytiles/utilities/c_parser/source_position.hpp"

namespace porytiles {

/// @brief Represents a parsed C pointer array declaration.
///
/// @details
/// ArrayDeclaration captures the structure of C pointer array declarations like:
/// @code
/// const u16 *const gTilesetAnims_General_Flower[] = {
///     gTilesetAnims_General_Flower_Frame0,
///     gTilesetAnims_General_Flower_Frame1,
///     gTilesetAnims_General_Flower_Frame2
/// };
/// @endcode
///
/// The parser extracts the array name and all identifier elements from the initializer list. This is used by
/// AnimCodeParser to extract animation frame sequences from tileset animation code.
///
/// @invariant name_ is never empty
/// @invariant position_ contains valid 1-based line and column numbers
class ArrayDeclaration {
  public:
    /// @brief Constructs an ArrayDeclaration.
    ///
    /// @param name The array variable name
    /// @param elements The identifier elements from the initializer list
    /// @param position The source position of the array name
    ArrayDeclaration(std::string name, std::vector<std::string> elements, SourcePosition position)
        : name_{std::move(name)}, elements_{std::move(elements)}, position_{position}
    {
    }

    /// @brief Returns the array variable name.
    ///
    /// @return A const reference to the name
    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

    /// @brief Returns the initializer list elements.
    ///
    /// @details
    /// Returns the identifiers from the array initializer in declaration order. For frame arrays, these are typically
    /// frame pointer names like "gTilesetAnims_General_Flower_Frame0".
    ///
    /// @return A const reference to the element vector
    [[nodiscard]] const std::vector<std::string> &elements() const
    {
        return elements_;
    }

    /// @brief Returns the source position of the array name.
    ///
    /// @return A const reference to the source position
    [[nodiscard]] const SourcePosition &position() const
    {
        return position_;
    }

  private:
    std::string name_;
    std::vector<std::string> elements_;
    SourcePosition position_;
};

} // namespace porytiles
