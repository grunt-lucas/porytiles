#pragma once

#include <regex>
#include <string>
#include <vector>

namespace porytiles {

/// @brief Regex-based include/exclude filter for diagnostic tags.
///
/// @details
/// DiagnosticTagFilter applies opt-in, regex-based filtering to diagnostic tags. The filtering logic:
/// 1. If the tag matches any exclude regex, it is suppressed (exclude overrides include).
/// 2. Otherwise, if the tag matches any include regex, it is shown.
/// 3. Otherwise, the tag is suppressed.
///
/// A filter with empty exclude and include lists suppresses all tags. To show everything, include a wildcard pattern
/// (".*"). Exclude patterns then re-exclude specific tags to override the include wildcard.
class DiagnosticTagFilter {
  public:
    /// @brief Constructs a DiagnosticTagFilter from string patterns.
    ///
    /// @details
    /// Each pattern string is compiled into a std::regex.
    ///
    /// @param exclude_patterns Regex patterns for tags to exclude (overrides includes)
    /// @param include_patterns Regex patterns for tags to include
    /// @pre All patterns in @p exclude_patterns and @p include_patterns must be valid regular expressions.
    DiagnosticTagFilter(std::vector<std::string> exclude_patterns, std::vector<std::string> include_patterns);

    /// @brief Determines whether a diagnostic with the given tag should be shown.
    ///
    /// @param tag The diagnostic categorization tag to check
    /// @return True if the diagnostic should be shown, false if it should be suppressed
    [[nodiscard]] bool should_show(const std::string &tag) const;

  private:
    // Note: std::regex is notoriously slow in libstdc++/libc++. If should_show() ever lands on a hot path, the first
    // things to consider are batch-compiling all patterns into a single alternation (pat1|pat2|pat3) or switching to a
    // faster engine (e.g. RE2, CTRE). Not a concern today given the low diagnostic count per compilation run.
    std::vector<std::regex> exclude_regexes_;
    std::vector<std::regex> include_regexes_;
};

} // namespace porytiles
