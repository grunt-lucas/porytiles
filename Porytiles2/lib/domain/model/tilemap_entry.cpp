#include "porytiles2/domain/model/tilemap_entry.hpp"

namespace porytiles2 {

bool TilemapEntry::is_transparent(const TilemapEntry &unused) const
{
    return tile_index_ == 0;
}

} // namespace porytiles2
