#pragma once

#include <cstddef>
#include <format>
#include <ostream>
#include <string>
#include <vector>

#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/string_utils.hpp"

namespace porytiles {

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

    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

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

} // namespace porytiles

template <>
struct std::formatter<porytiles::PaletteHint> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::PaletteHint &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};

template <>
struct std::formatter<std::vector<porytiles::PaletteHint>> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const std::vector<porytiles::PaletteHint> &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};
