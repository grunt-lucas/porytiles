#pragma once

#include <concepts>

namespace porytiles2 {

/**
 * @brief Concept that requires a type to support both intrinsic and extrinsic transparency checks.
 *
 * @details
 * A type satisfies this concept if it has an `is_transparent()` method that returns a bool value. This transparency
 * could be either an "intrinsic" or "extrinsic" transparency. Intrinsic; that is, it can be computed from internal
 * properties of the type in question. Extrinsic; that is, it matches a user-supplied transparency value, even if the
 * type isn't "intrinsically" transparent. Here, the user supplied "extrinsic" value is the one passed in to
 * `is_transparent`.
 */
template <typename T>
concept SupportsTransparency = requires(const T &t) {
    { t.is_transparent(t) } -> std::convertible_to<bool>;
};

} // namespace porytiles2
