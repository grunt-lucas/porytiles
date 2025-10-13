#pragma once

#include <source_location>
#include <string>

namespace porytiles2 {

/**
 * @brief Extracts the function name from a source location.
 *
 * @details
 * This function extracts the function name from a std::source_location and parses it
 * to return just the simple function name without qualifiers, parameters, or return type.
 *
 * For example, given:
 * - GCC: "std::size_t porytiles2::LazyLayeredConfig::num_tiles_primary(const std::string&) const"
 * - Clang: "porytiles2::LazyLayeredConfig::num_tiles_primary"
 *
 * Both would return: "num_tiles_primary"
 *
 * @param location The source location from which to extract the function name (defaults to caller's location)
 * @return The simple function name without qualifiers
 */
[[nodiscard]] std::string extract_function_name(const std::source_location &location = std::source_location::current());

} // namespace porytiles2
