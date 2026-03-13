#pragma once

#include <regex>
#include <string>
#include <vector>

namespace porytiles2 {

/**
 * @brief Regex-based include/exclude filter for diagnostic tags.
 *
 * @details
 * DiagnosticTagFilter applies regex-based filtering to diagnostic tags. The filtering logic is:
 * 1. If the tag matches any exclude regex, it is suppressed (unless step 2 overrides).
 * 2. If the tag matches any include regex, it is shown (overrides exclude).
 * 3. If no exclude regex matches, the tag is shown by default.
 *
 * A filter with empty exclude and include lists passes all tags through (no-op filter).
 */
class DiagnosticTagFilter {
  public:
    /**
     * @brief Constructs a DiagnosticTagFilter from string patterns.
     *
     * @details
     * Each pattern string is compiled into a std::regex.
     *
     * @param exclude_patterns Regex patterns for tags to exclude
     * @param include_patterns Regex patterns for tags to include (overrides excludes)
     * @pre All patterns in @p exclude_patterns and @p include_patterns must be valid regular expressions.
     */
    DiagnosticTagFilter(std::vector<std::string> exclude_patterns, std::vector<std::string> include_patterns);

    /**
     * @brief Determines whether a diagnostic with the given tag should be shown.
     *
     * @param tag The diagnostic categorization tag to check
     * @return True if the diagnostic should be shown, false if it should be suppressed
     */
    [[nodiscard]] bool should_show(const std::string &tag) const;

  private:
    /*
     * TODO: std::regex is notoriously slow in libstdc++/libc++. If should_show() ever lands on a hot path, consider
     * batch-compiling all patterns into a single alternation (pat1|pat2|pat3) or switching to a faster engine (e.g.
     * RE2, CTRE). Not a concern today given the low diagnostic count per compilation run.
     */
    std::vector<std::regex> exclude_regexes_;
    std::vector<std::regex> include_regexes_;
};

} // namespace porytiles2
