#pragma once

#include <expected>
#include <string>

namespace porytiles2 {

/**
 * @brief A result with some type `T` on success, otherwise an error of type `E`.
 *
 * @details
 * Many Porytiles operations need to return either an expected result or some description of what went wrong during
 * result computation. This type alias is a convenient wrapper for the stdlib `std::expected`, which provides this exact
 * functionality. The `std::string` error type will typically be some description of what went wrong. However, the alias
 * supports a custom user type for the error type if a string is not sufficient.
 *
 * @tparam T The type of the expected result
 * @tparam E The error type, defaults to `std::string`
 */
template <typename T, typename E = std::string>
using Result = std::expected<T, E>;

} // namespace porytiles2
