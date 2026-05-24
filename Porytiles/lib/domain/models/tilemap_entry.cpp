#include "porytiles/domain/models/tilemap_entry.hpp"

namespace porytiles {

bool TilemapEntry::is_transparent() const
{
    return tile_index_ == 0;
}

} // namespace porytiles
