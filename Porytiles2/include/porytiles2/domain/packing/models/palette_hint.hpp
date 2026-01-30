#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace porytiles2 {

/**
 * @brief Represents a palette hint for the palette packing algorithm.
 *
 * @details
 * A PaletteHint contains a named palette that provides guidance to the packing algorithm about how colors should be
 * grouped together. The name allows the hint to be identified and referenced.
 */
class PaletteHint {
  public:
    /**
     * @brief Constructs a PaletteHint with a name and palette.
     *
     * @param name The name of this palette hint
     * @param pal The palette containing the hint colors
     */
    PaletteHint(std::string name, Palette<Rgba32> pal) : name_{std::move(name)}, pal_{std::move(pal)} {}

    /**
     * @brief Gets the name of this palette hint.
     *
     * @return A const reference to the name
     */
    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

    /**
     * @brief Gets the palette of this palette hint.
     *
     * @return A const reference to the palette
     */
    [[nodiscard]] const Palette<Rgba32> &pal() const
    {
        return pal_;
    }

  private:
    std::string name_;
    Palette<Rgba32> pal_;
};

/**
 * @brief Converts a PaletteHint to its string representation.
 *
 * @details
 * Returns the name of the palette hint, which serves as its unique identifier.
 *
 * @param hint The PaletteHint to convert
 * @return The name of the palette hint
 */
inline std::string to_string(const PaletteHint &hint)
{
    return hint.name();
}

/**
 * @brief Stream output operator for PaletteHint.
 *
 * @details
 * Outputs the name of the palette hint to the stream.
 *
 * @param os The output stream
 * @param hint The PaletteHint to output
 * @return Reference to the output stream
 */
inline std::ostream &operator<<(std::ostream &os, const PaletteHint &hint)
{
    return os << to_string(hint);
}

} // namespace porytiles2
