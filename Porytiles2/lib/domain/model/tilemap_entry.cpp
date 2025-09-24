#include "porytiles2/domain/model/tilemap_entry.hpp"

#include <set>

namespace porytiles2 {

bool TilemapEntry::is_transparent(const std::set<TilemapEntry> &unused) const
{
    return tile_index_ == 0;
}

} // namespace porytiles2
