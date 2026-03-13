#include "porytiles2/xcut/diagnostics/diagnostic_tag_filter.hpp"

#include <algorithm>

namespace porytiles2 {

DiagnosticTagFilter::DiagnosticTagFilter(
    std::vector<std::string> exclude_patterns, std::vector<std::string> include_patterns)
{
    /*
     * TODO: Catch std::regex_error here and emit a clear diagnostic (e.g. via FormattableError) instead of letting the
     * exception propagate as an opaque crash. A malformed regex in the user's YAML config should produce a message
     * like: "Invalid regex in diagnostics.warnings.exclude: '[unclosed'".
     */
    exclude_regexes_.reserve(exclude_patterns.size());
    for (const auto &pattern : exclude_patterns) {
        exclude_regexes_.emplace_back(pattern);
    }

    include_regexes_.reserve(include_patterns.size());
    for (const auto &pattern : include_patterns) {
        include_regexes_.emplace_back(pattern);
    }
}

/*
 * TODO: regex_search performs substring matching, so an exclude pattern of "pal" matches "palette-overflow",
 * "opal-thing", and "principal". Users must anchor with ^/$ for exact matches. This is intentional (grep-like
 * semantics) but should be documented for users to avoid surprises.
 */
bool DiagnosticTagFilter::should_show(const std::string &tag) const
{
    bool excluded =
        std::ranges::any_of(exclude_regexes_, [&tag](const std::regex &re) { return std::regex_search(tag, re); });

    if (!excluded) {
        return true;
    }

    // Check if an include pattern overrides the exclusion
    bool included =
        std::ranges::any_of(include_regexes_, [&tag](const std::regex &re) { return std::regex_search(tag, re); });

    return included;
}

} // namespace porytiles2
