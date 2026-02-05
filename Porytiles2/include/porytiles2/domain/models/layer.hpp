#pragma once

#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <string>

#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Specifies whether a metatile uses dual-layer or triple-layer mode.
 *
 * @details
 * In dual-layer mode, a metatile uses 8 tile entries (bottom and middle layers). In triple-layer mode, a metatile uses
 * 12 tile entries (bottom, middle, and top layers).
 */
enum class LayerMode { dual, triple };

/**
 * @brief Converts a numeric value to LayerMode.
 *
 * @details
 * Accepts 8 for dual-layer mode or 12 for triple-layer mode.
 *
 * @param s The numeric value (must be 8 or 12)
 * @return The corresponding LayerMode
 */
[[nodiscard]] inline LayerMode layer_mode_from_val(std::size_t s)
{
    if (s == 8) {
        return LayerMode::dual;
    }
    if (s == 12) {
        return LayerMode::triple;
    }
    panic("invalid LayerMode integer: " + std::to_string(s));
}

/**
 * @brief Converts a string to LayerMode.
 *
 * @param s The string to convert (must be "dual" or "triple")
 * @return The corresponding LayerMode, or std::nullopt if the string is invalid
 */
[[nodiscard]] inline std::optional<LayerMode> layer_mode_from_str(const std::string &s)
{
    if (s == "dual") {
        return LayerMode::dual;
    }
    if (s == "triple") {
        return LayerMode::triple;
    }
    return std::nullopt;
}

/**
 * @brief Converts LayerMode to string representation.
 *
 * @param mode The LayerMode to convert
 * @return String representation ("dual" or "triple")
 */
[[nodiscard]] inline std::string to_string(LayerMode mode)
{
    switch (mode) {
    case LayerMode::dual:
        return "dual";
    case LayerMode::triple:
        return "triple";
    }
    panic("unhandled LayerMode value");
}

/**
 * @brief Stream insertion operator for LayerMode.
 *
 * @param os The output stream
 * @param mode The LayerMode to output
 * @return The output stream
 */
inline std::ostream &operator<<(std::ostream &os, const LayerMode mode)
{
    return os << to_string(mode);
}

/**
 * @brief Specifies which layers of a metatile are used for rendering.
 *
 * @details
 * - normal: Uses middle and top layers
 * - covered: Uses bottom and middle layers
 * - split: Uses bottom and top layers
 */
enum class LayerType : unsigned int { normal = 0, covered = 1, split = 2 };

/**
 * @brief Converts LayerType to string representation.
 *
 * @param layer_type The LayerType to convert
 * @return Human-readable string describing the layer type
 */
[[nodiscard]] inline std::string to_string(LayerType layer_type)
{
    switch (layer_type) {
    case LayerType::normal:
        return "Normal - Middle/Top";
    case LayerType::covered:
        return "Covered - Bottom/Middle";
    case LayerType::split:
        return "Split - Bottom/Top";
    default:
        panic("to_string(LayerType) unknown LayerType");
    }
}

/**
 * @brief Converts an integer to LayerType.
 *
 * @param i The integer value (must be 0, 1, or 2)
 * @return ChainableResult containing the LayerType or an error
 */
[[nodiscard]] inline ChainableResult<LayerType> layer_type_from_int(unsigned int i)
{
    if (i > static_cast<unsigned int>(LayerType::split)) {
        return FormattableError{
            "invalid layer type integer value '{}': must be 0, 1, or 2", FormatParam{i, Style::bold}};
    }
    return static_cast<LayerType>(i);
}

} // namespace porytiles2

template <>
struct std::formatter<porytiles2::LayerMode> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles2::LayerMode &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles2::to_string(value));
    }
};

template <>
struct std::formatter<porytiles2::LayerType> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles2::LayerType &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles2::to_string(value));
    }
};
