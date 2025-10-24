#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

#include <string>
#include <vector>

#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/result/error.hpp"

namespace porytiles2 {

void BufferedUserDiagnostics::note(const std::string &tag, const std::vector<std::string> &lines) const
{
    notes_.push_back(lines);
    note_tag_counts_[tag]++;
}

void BufferedUserDiagnostics::warn_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    warn_notes_.push_back(lines);
    warn_note_tag_counts_[tag]++;
}

void BufferedUserDiagnostics::warn(const std::string &tag, const std::vector<std::string> &lines) const
{
    warnings_.push_back(lines);
    warning_tag_counts_[tag]++;
}

void BufferedUserDiagnostics::err(const std::string &tag, const std::vector<std::string> &lines) const
{
    errors_.push_back(lines);
    error_tag_counts_[tag]++;
}

void BufferedUserDiagnostics::emit_fatal_proximate(const Error &err) const
{
    PlainTextFormatter formatter{};
    fatal_proximates_.push_back(err.details(formatter));
}

void BufferedUserDiagnostics::emit_fatal_step(const Error &err) const
{
    PlainTextFormatter formatter{};
    fatal_steps_.push_back(err.details(formatter));
}

void BufferedUserDiagnostics::emit_fatal_root(const Error &err) const
{
    PlainTextFormatter formatter{};
    fatal_roots_.push_back(err.details(formatter));
}

} // namespace porytiles2
