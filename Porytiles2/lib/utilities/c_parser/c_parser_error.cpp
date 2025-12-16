#include "porytiles2/utilities/c_parser/c_parser_error.hpp"

#include <string>
#include <vector>

#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

std::vector<std::string> CParserError::details(const TextFormatter &formatter) const
{
    return {formatter.format("line {}, column {}: {}", position_.line, position_.column, message_)};
}

std::unique_ptr<Error> CParserError::clone() const
{
    return std::make_unique<CParserError>(position_, message_);
}

} // namespace porytiles2
