#include "porytiles2/domain/model/porymap_tileset_component.hpp"

#include <utility>

#include "porytiles2/domain/model/vram_metatile.hpp"

namespace porytiles2 {

void PorymapTilesetComponent::push_back_tilemap_entry(TilemapEntry entry)
{
    metatiles_bin_.push_back(std::move(entry));
}

void PorymapTilesetComponent::push_back_pal(RgbaPal pal)
{
    pals_.push_back(std::move(pal));
}

bool PorymapTilesetComponent::is_empty() const
{
    return metatiles_bin_.empty();
}

} // namespace porytiles2
