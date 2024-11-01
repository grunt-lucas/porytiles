#ifndef PORYTILES_COLOR_COLOR_CONVERTER_H
#define PORYTILES_COLOR_COLOR_CONVERTER_H

#include <porytiles/Color/RGBLike.h>

namespace porytiles::color {

/**
 * @brief Service interface for a converter service that transforms one Color implementation into a
 * different Color implementation.
 */
template<RGBLike T, RGBLike U>
class ColorConverter {
  public:
    virtual ~ColorConverter() = default;
    [[nodiscard]] virtual U convert(const T&) const = 0;
};

} // namespace porytiles::color

#endif // PORYTILES_COLOR_COLOR_CONVERTER_H