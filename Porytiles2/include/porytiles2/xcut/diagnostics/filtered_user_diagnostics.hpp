#pragma once

#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/utilities/result/error.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/diagnostic_tag_filter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Decorator that applies tag-based filtering to any UserDiagnostics implementation.
 *
 * @details
 * FilteredUserDiagnostics wraps an inner UserDiagnostics and applies DiagnosticTagFilter-based
 * filtering to warnings and remarks. Errors and fatal diagnostics always pass through unfiltered,
 * since they represent conditions that require user attention regardless of filter settings.
 *
 * When a warning or remark is filtered out, its associated notes are also suppressed.
 */
class FilteredUserDiagnostics final : public UserDiagnostics {
  public:
    /**
     * @brief Constructs a FilteredUserDiagnostics decorator.
     *
     * @param format The text formatter (passed to base class)
     * @param inner The inner UserDiagnostics to delegate to after filtering
     * @param warning_filter Filter to apply to warning diagnostics
     * @param remark_filter Filter to apply to remark diagnostics
     * @pre @p inner must outlive the FilteredUserDiagnostics instance.
     */
    FilteredUserDiagnostics(
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> inner,
        DiagnosticTagFilter warning_filter,
        DiagnosticTagFilter remark_filter);

    void remark(const std::string &tag, const std::vector<std::string> &lines) const override;
    void remark_note(const std::string &tag, const std::vector<std::string> &lines) const override;

    void warning(const std::string &tag, const std::vector<std::string> &lines) const override;
    void warning_note(const std::string &tag, const std::vector<std::string> &lines) const override;

    void error(const std::string &tag, const std::vector<std::string> &lines) const override;
    void error_note(const std::string &tag, const std::vector<std::string> &lines) const override;

    void emit_fatal_proximate(const Error &err) const override;
    void emit_fatal_step(const Error &err) const override;
    void emit_fatal_root(const Error &err) const override;

  private:
    gsl::not_null<const UserDiagnostics *> inner_;
    DiagnosticTagFilter warning_filter_;
    DiagnosticTagFilter remark_filter_;
};

} // namespace porytiles2
