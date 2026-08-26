#include "porytiles/xcut/diagnostics/diagnostic_tag_filter.hpp"

#include <algorithm>

namespace porytiles {

DiagnosticTagFilter::DiagnosticTagFilter(
    std::vector<std::string> exclude_patterns, std::vector<std::string> include_patterns)
{
    exclude_regexes_.reserve(exclude_patterns.size());
    for (const auto &pattern : exclude_patterns) {
        exclude_regexes_.emplace_back(pattern);
    }

    include_regexes_.reserve(include_patterns.size());
    for (const auto &pattern : include_patterns) {
        include_regexes_.emplace_back(pattern);
    }
}

bool DiagnosticTagFilter::should_show(const std::string &tag) const
{
    bool excluded =
        std::ranges::any_of(exclude_regexes_, [&tag](const std::regex &re) { return std::regex_search(tag, re); });

    if (excluded) {
        return false;
    }

    // Diagnostics are opt-in, tags must match an include pattern to be shown
    bool included =
        std::ranges::any_of(include_regexes_, [&tag](const std::regex &re) { return std::regex_search(tag, re); });

    return included;
}

} // namespace porytiles
