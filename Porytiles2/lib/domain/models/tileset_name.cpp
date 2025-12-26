#include "porytiles2/domain/models/tileset_name.hpp"

namespace porytiles2 {

ChainableResult<TilesetName> TilesetName::from(const std::string &name)
{
    if (!name.starts_with(prefix)) {
        return FormattableError{
            "invalid tileset name '{}', must begin with prefix '{}'",
            FormatParam{name, Style::bold},
            FormatParam{prefix, Style::bold}};
    }
    return TilesetName{name};
}

ChainableResult<TilesetName> TilesetName::from_shorthand(const std::string &shorthand)
{
    if (shorthand.starts_with(prefix)) {
        return FormattableError{
            "tileset name shorthand may not have reserved prefix '{}'", FormatParam{prefix, Style::bold}};
    }
    return TilesetName{prefix + shorthand};
}

std::string TilesetName::shorthand() const
{
    return trim_prefix(prefix, name_);
}

} // namespace porytiles2
