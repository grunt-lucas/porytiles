#ifndef PORYTILES_COLOR_RGBLIKE_H
#define PORYTILES_COLOR_RGBLIKE_H

#include <concepts>
#include <cstdint>
#include <string>

namespace porytiles::color {

/**
 * @brief An RGBLike should match any type which can behave like an RGBA format color.
 *
 * @details
 * The Porytiles library is color implementation agnostic. That is, it only needs a color to conform
 * to this simple concept. The underlying color implementation is irrelevant, as most color
 * representations can be converted into red, green, and blue channels via a simple formula.
 * Porytiles provides a few RGBLike compliant implementations that may be useful, but users of the
 * Porytiles library may also implement their own if they have a specific use-case.
 */
template <typename T>
concept RGBLike = requires(T t) {
    {
        t.computeRedComponent()
    } -> std::same_as<std::uint8_t>;
    {
        t.computeGreenComponent()
    } -> std::same_as<std::uint8_t>;
    {
        t.computeBlueComponent()
    } -> std::same_as<std::uint8_t>;
    {
        t.computeAlphaComponent()
    } -> std::same_as<std::uint8_t>;
    {
        t.toJascString()
    } -> std::same_as<std::string>;
};

/**
 * @brief Completely transparent alpha channel value.
 */
constexpr std::uint8_t ALPHA_TRANSPARENT = 0;

/**
 * @brief Completely opaque alpha channel value.
 */
constexpr std::uint8_t ALPHA_OPAQUE = 0xff;

} // namespace porytiles::color

#endif // PORYTILES_COLOR_RGBLIKE_H