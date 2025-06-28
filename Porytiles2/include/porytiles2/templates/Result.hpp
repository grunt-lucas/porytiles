#pragma once

#include <expected>
#include <string>

namespace porytiles {

/**
 * @brief Alias for `std::expected<T, std::string>`, where `T` is some type.
 *
 * @details
 * Many Porytiles operations need to return either an expected result or some
 * description of what went wrong during result computation. This type alias is
 * a convenient wrapper for the stdlib `std::expected`, which provides this
 * exact functionality. The `std::string` error type will typically be some
 * description of what went wrong.
 *
 * @tparam T The type of the expected result.
 */
template <typename T> using Result = std::expected<T, std::string>;

} // namespace porytiles
