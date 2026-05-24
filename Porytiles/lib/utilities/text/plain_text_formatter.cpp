#include "porytiles/utilities/text/plain_text_formatter.hpp"

#include <string>

namespace porytiles {

std::string PlainTextFormatter::style(const std::string &text, Style styles) const
{
    // no style text is applied
    return text;
}

} // namespace porytiles
