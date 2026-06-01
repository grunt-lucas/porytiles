#include "porytiles/utilities/source_locations.hpp"

#include <algorithm>
#include <string_view>

namespace porytiles {

std::string extract_function_name(const std::source_location &location)
{
    std::string_view full_function_name = location.function_name();

    // Handle empty input
    if (full_function_name.empty()) {
        return "";
    }

    // Find the opening parenthesis (if it exists, we have a GCC-style signature)
    const auto paren_pos = full_function_name.find('(');
    std::string_view qualified_name;

    if (paren_pos != std::string_view::npos) {
        // GCC-style: "return_type namespace::class::function(params) qualifiers"
        // We need to extract "namespace::class::function"
        qualified_name = full_function_name.substr(0, paren_pos);

        // Trim trailing whitespace
        while (!qualified_name.empty() && std::isspace(qualified_name.back())) {
            qualified_name.remove_suffix(1);
        }

        // Now we need to find where the qualified name starts (after the return type)
        // Look for the last space before the qualified name
        const auto last_space = qualified_name.find_last_of(' ');
        if (last_space != std::string_view::npos) {
            qualified_name = qualified_name.substr(last_space + 1);
        }
    }
    else {
        // Clang-style: "namespace::class::function"
        qualified_name = full_function_name;

        // Trim any trailing whitespace
        while (!qualified_name.empty() && std::isspace(qualified_name.back())) {
            qualified_name.remove_suffix(1);
        }
    }

    // Now extract the simple name (everything after the last ::)
    const auto last_colon = qualified_name.rfind("::");
    if (last_colon != std::string_view::npos) {
        qualified_name = qualified_name.substr(last_colon + 2);
    }

    return std::string{qualified_name};
}

} // namespace porytiles
