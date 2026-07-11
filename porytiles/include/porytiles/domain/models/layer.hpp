#pragma once

#include <cctype>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <string>

#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief Specifies whether a metatile uses dual-layer or triple-layer mode.
///
/// @details
/// In dual-layer mode, a metatile uses 8 tile entries (bottom and middle layers). In triple-layer mode, a metatile uses
/// 12 tile entries (bottom, middle, and top layers).
enum class LayerMode { dual, triple };

/// @brief Converts a numeric value to LayerMode.
///
/// @details
/// Accepts 8 for dual-layer mode or 12 for triple-layer mode.
///
/// @param s The numeric value (must be 8 or 12)
/// @return The corresponding LayerMode
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

/// @brief Converts a string to LayerMode.
///
/// @param s The string to convert (must be "dual" or "triple")
/// @return The corresponding LayerMode, or std::nullopt if the string is invalid
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

/// @brief Converts LayerMode to string representation.
///
/// @param mode The LayerMode to convert
/// @return String representation ("dual" or "triple")
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

/// @brief Stream insertion operator for LayerMode.
///
/// @param os The output stream
/// @param mode The LayerMode to output
/// @return The output stream
inline std::ostream &operator<<(std::ostream &os, const LayerMode mode)
{
    return os << to_string(mode);
}

/// @brief Specifies which layers of a metatile are used for rendering.
///
/// @details
/// - normal: Uses middle and top layers
/// - covered: Uses bottom and middle layers
/// - split: Uses bottom and top layers
enum class LayerType : unsigned int { normal = 0, covered = 1, split = 2 };

/// @brief Converts LayerType to string representation.
///
/// @param layer_type The LayerType to convert
/// @return Human-readable string describing the layer type
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

/// @brief Converts an integer to LayerType.
///
/// @param i The integer value (must be 0, 1, or 2)
/// @return ChainableResult containing the LayerType or an error
[[nodiscard]] inline ChainableResult<LayerType> layer_type_from_int(unsigned int i)
{
    if (i > static_cast<unsigned int>(LayerType::split)) {
        return FormattableError{
            "invalid layer type integer value '{}': must be 0, 1, or 2", FormatParam{i, Style::bold}};
    }
    return static_cast<LayerType>(i);
}

/// @brief Converts a LayerType to its lowercase CSV token.
///
/// @details
/// These tokens ("normal", "covered", "split") are the stable, machine-readable form used in the attributes CSV
/// layer_type column. They are distinct from to_string(LayerType), which returns a human-readable display string
/// ("Normal - Middle/Top") unsuitable as a round-trippable token.
///
/// @param layer_type The LayerType to convert
/// @return The CSV token: "normal", "covered", or "split"
[[nodiscard]] inline std::string layer_type_csv_token(LayerType layer_type)
{
    switch (layer_type) {
    case LayerType::normal:
        return "normal";
    case LayerType::covered:
        return "covered";
    case LayerType::split:
        return "split";
    default:
        panic("layer_type_csv_token unknown LayerType");
    }
}

/// @brief Parses a CSV layer_type token into a LayerType.
///
/// @details
/// Matching is case-insensitive, so "Normal", "NORMAL", and "normal" all parse. An unrecognized token is a hard error
/// listing the valid tokens.
///
/// @param token The token to parse
/// @return The parsed LayerType, or an error naming the valid tokens
[[nodiscard]] inline ChainableResult<LayerType> layer_type_from_csv_token(const std::string &token)
{
    std::string lower;
    lower.reserve(token.size());
    for (const unsigned char c : token) {
        lower.push_back(static_cast<char>(std::tolower(c)));
    }

    if (lower == "normal") {
        return LayerType::normal;
    }
    if (lower == "covered") {
        return LayerType::covered;
    }
    if (lower == "split") {
        return LayerType::split;
    }
    return FormattableError{
        "invalid layer_type token '{}': must be 'normal', 'covered', or 'split'", FormatParam{token, Style::bold}};
}

} // namespace porytiles

template <>
struct std::formatter<porytiles::LayerMode> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::LayerMode &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};

template <>
struct std::formatter<porytiles::LayerType> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::LayerType &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};
