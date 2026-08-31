#include "porytiles/domain/services/tileset_name_resolver.hpp"

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "gsl/pointers"

#include "porytiles/utilities/dynamic_cased_name.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles;

/// @brief Case-folds a name into the key used for fuzzy comparison.
std::string fuzzy_key(const std::string &name)
{
    return DynamicCasedName{name}.canonical();
}

} // namespace

namespace porytiles {

ChainableResult<std::string> resolve_tileset_name(
    const std::string &input, const std::set<std::string> &tileset_names, gsl::not_null<const TextFormatter *> format)
{
    // An exact match is never fuzzy-decoded, so fuzzy-equal siblings each stay addressable by their exact name.
    if (tileset_names.contains(input)) {
        return input;
    }

    const std::string input_key = fuzzy_key(extract_tileset_shorthand(input));

    std::vector<std::string> matches;
    if (!input_key.empty()) {
        for (const auto &tileset_name : tileset_names) {
            // Match against both the shorthand and full-name keys, so inputs that mangle the prefix's own casing
            // (e.g. "gtileset_secret_base") still resolve.
            if (fuzzy_key(extract_tileset_shorthand(tileset_name)) == input_key ||
                fuzzy_key(tileset_name) == input_key) {
                matches.push_back(tileset_name);
            }
        }
    }

    if (matches.size() == 1) {
        return matches.front();
    }

    if (matches.empty()) {
        return FormattableError{
            "Tileset name '{}' does not match any tileset in this project.", FormatParam{input, Style::bold}};
    }

    std::vector<std::string> lines;
    lines.push_back(format->format("Tileset name '{}' is ambiguous.", FormatParam{input, Style::bold}));
    lines.emplace_back("");
    lines.emplace_back("It matches all of the following tilesets:");
    for (const auto &match : matches) {
        lines.push_back(format->format("  - '{}'", FormatParam{match, Style::bold}));
    }
    lines.emplace_back("");
    lines.emplace_back("Use the exact tileset name to disambiguate.");
    return FormattableError{std::move(lines)};
}

} // namespace porytiles
