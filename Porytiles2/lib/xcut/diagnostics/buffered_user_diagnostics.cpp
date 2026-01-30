#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

#include <string>
#include <vector>

#include "porytiles2/utilities/result/error.hpp"

namespace porytiles2 {

void BufferedUserDiagnostics::remark(const std::string &tag, const std::vector<std::string> &lines) const
{
    remarks_.push_back(lines);
    remark_tag_counts_[tag]++;
}

void BufferedUserDiagnostics::remark_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    remark_notes_.push_back(lines);
    remark_note_tag_counts_[tag]++;
}

void BufferedUserDiagnostics::warning_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    warning_notes_.push_back(lines);
    warning_note_tag_counts_[tag]++;
}

void BufferedUserDiagnostics::error_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    error_notes_.push_back(lines);
    error_note_tag_counts_[tag]++;
}

void BufferedUserDiagnostics::warning(const std::string &tag, const std::vector<std::string> &lines) const
{
    warnings_.push_back(lines);
    warning_tag_counts_[tag]++;
}

void BufferedUserDiagnostics::error(const std::string &tag, const std::vector<std::string> &lines) const
{
    errors_.push_back(lines);
    error_tag_counts_[tag]++;
}

void BufferedUserDiagnostics::emit_fatal_proximate(const Error &err) const
{
    fatal_proximates_.push_back(err.details(formatter()));
}

void BufferedUserDiagnostics::emit_fatal_step(const Error &err) const
{
    fatal_steps_.push_back(err.details(formatter()));
}

void BufferedUserDiagnostics::emit_fatal_root(const Error &err) const
{
    fatal_roots_.push_back(err.details(formatter()));
}

} // namespace porytiles2
