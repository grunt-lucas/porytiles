#pragma once

#include <concepts>

namespace porytiles {

/// @brief Concept that requires a type to support transparency checks.
///
/// @details
/// A type satisfies this concept if it has an `is_transparent()` method that returns a bool value. This method
/// can either:
/// - Take no parameters (intrinsic transparency only, e.g., IndexPixel where index 0 is always transparent)
/// - Take a parameter of the same type (extrinsic transparency, e.g., Rgba32 where a color can match an external
///   transparency value)
///
/// Tile and container types use requires clauses to provide only the appropriate overload(s) based on the pixel type.
///
/// **Design Invariant:** Types implementing this concept should default-construct to their transparent representation.
/// That is, `T{}` should produce a transparent pixel value. This enables generic code to create transparent pixels
/// without knowing the specific pixel type (e.g., `Rgba32{}` and `IndexPixel{}` both default-construct to transparent
/// values).
template <typename T>
concept SupportsTransparency = requires(const T &t) {
    { t.is_transparent() } -> std::convertible_to<bool>;
} || requires(const T &t) {
    { t.is_transparent(t) } -> std::convertible_to<bool>;
};
} // namespace porytiles
