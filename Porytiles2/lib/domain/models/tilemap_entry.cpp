#include "porytiles2/domain/models/tilemap_entry.hpp"

namespace porytiles2 {

bool TilemapEntry::is_transparent() const
{
    return tile_index_ == 0;
}

} // namespace porytiles2
