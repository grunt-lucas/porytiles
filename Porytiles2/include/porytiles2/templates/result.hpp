#pragma once

#include <expected>
#include <string>

namespace porytiles {

/**
 * @brief Alias for `std::expected<T, std::string>`, where `T` is some type.
 *
 * @tparam T The type of the expected result.
 */
template <typename T>
using Result = std::expected<T, std::string>;

} // namespace porytiles
