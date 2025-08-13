#pragma once

#include "porytiles2/domain/model/metatile.hpp"
#include "porytiles2/domain/model/tilemap_entry.hpp"

namespace porytiles2 {

// TODO: this isn't right, VramMetatile can't directly inherit from Metatile since Metatile forces the underlying
// tiles to be 8x8 with the given pixel type. But here, we want a simple three layers of 2x2 metatile entries. So we
// either need to update Tile so that it can have a custom dimension, or update the VramMetatile/Metatile relationship.
class VramMetatile : public Metatile<TilemapEntry> {
  public:
    VramMetatile() = default;
};

} // namespace porytiles2
