#include "porytiles2/xcut/diagnostics/filtered_user_diagnostics.hpp"

namespace porytiles2 {

FilteredUserDiagnostics::FilteredUserDiagnostics(
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> inner,
    DiagnosticTagFilter warning_filter,
    DiagnosticTagFilter remark_filter)
    : UserDiagnostics{format}, inner_{inner}, warning_filter_{std::move(warning_filter)},
      remark_filter_{std::move(remark_filter)}
{
}

void FilteredUserDiagnostics::remark(const std::string &tag, const std::vector<std::string> &lines) const
{
    if (remark_filter_.should_show(tag)) {
        inner_->remark(tag, lines);
    }
}

/*
 * TODO: remark_note and warning_note independently re-check the filter, which assumes notes always carry the same tag
 * as their parent diagnostic. This is an implicit invariant — if any code path ever emits warning("tag-A") followed by
 * warning_note("tag-B"), the note could be shown/hidden independently. A stateful approach (e.g. a "last-shown" flag
 * set by warning/remark that note methods consult) would make the coupling explicit and more robust.
 */
void FilteredUserDiagnostics::remark_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    if (remark_filter_.should_show(tag)) {
        inner_->remark_note(tag, lines);
    }
}

void FilteredUserDiagnostics::warning(const std::string &tag, const std::vector<std::string> &lines) const
{
    if (warning_filter_.should_show(tag)) {
        inner_->warning(tag, lines);
    }
}

void FilteredUserDiagnostics::warning_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    if (warning_filter_.should_show(tag)) {
        inner_->warning_note(tag, lines);
    }
}

void FilteredUserDiagnostics::error(const std::string &tag, const std::vector<std::string> &lines) const
{
    inner_->error(tag, lines);
}

void FilteredUserDiagnostics::error_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    inner_->error_note(tag, lines);
}

void FilteredUserDiagnostics::emit_fatal_proximate(const Error &err) const
{
    inner_->emit_fatal_proximate(err);
}

void FilteredUserDiagnostics::emit_fatal_step(const Error &err) const
{
    inner_->emit_fatal_step(err);
}

void FilteredUserDiagnostics::emit_fatal_root(const Error &err) const
{
    inner_->emit_fatal_root(err);
}

} // namespace porytiles2
