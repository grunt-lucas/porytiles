#include "porytiles2/domain/models/tileset_name.hpp"

namespace porytiles2 {

TilesetName TilesetName::from(const std::string &name)
{
    if (name.starts_with(prefix)) {
        return TilesetName{name};
    }
    return TilesetName{prefix + name};
}

std::string TilesetName::shorthand() const
{
    return trim_prefix(name_, prefix);
}

} // namespace porytiles2
