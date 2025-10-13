#ifndef PORYTILES2_UTILITIES_SOURCE_LOCATIONS_HPP
#define PORYTILES2_UTILITIES_SOURCE_LOCATIONS_HPP

#include <source_location>
#include <string>
#include <string_view>

namespace porytiles2 {

/**
 * @brief Extracts the simple function name from a full function signature.
 *
 * @details
 * This function parses the output of std::source_location::current().function_name()
 * to extract just the simple function name without qualifiers, parameters, or return type.
 *
 * For example, given:
 * - GCC: "std::size_t porytiles2::LazyLayeredConfig::num_tiles_primary(const std::string&) const"
 * - Clang: "porytiles2::LazyLayeredConfig::num_tiles_primary"
 *
 * Both would return: "num_tiles_primary"
 *
 * @param full_function_name The full function signature from std::source_location::function_name()
 * @return The simple function name without qualifiers
 */
[[nodiscard]] std::string extract_simple_function_name(std::string_view full_function_name);

} // namespace porytiles2

#endif // PORYTILES2_UTILITIES_SOURCE_LOCATIONS_HPP
