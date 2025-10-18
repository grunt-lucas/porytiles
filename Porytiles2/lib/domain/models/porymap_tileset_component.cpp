#include "porytiles2/domain/models/porymap_tileset_component.hpp"

#include <utility>

#include "fmt/format.h"

#include "porytiles2/domain/models/rgba_pal.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

void PorymapTilesetComponent::push_back_tilemap_entry(TilemapEntry entry)
{
    // std::move here even though TilemapEntry is trivially-copyable, in case it changes later
    metatiles_bin_.push_back(std::move(entry));
}

void PorymapTilesetComponent::push_back_attribute(MetatileAttribute attribute)
{
    metatile_attributes_.push_back(std::move(attribute));
}

void PorymapTilesetComponent::set_pal(RgbaPal pal, int pal_index)
{
    if (pal_index < 0 || pal_index >= pal::num_pals) {
        panic(fmt::format("invalid pal index {}: out of range", pal_index));
    }
    pals_[pal_index] = std::move(pal);
}

const RgbaPal &PorymapTilesetComponent::pal_at(int pal_index) const
{
    if (pal_index < 0 || pal_index >= pal::num_pals) {
        panic(fmt::format("invalid pal index {}: out of range", pal_index));
    }
    return pals_[pal_index];
}

bool PorymapTilesetComponent::is_empty() const
{
    return metatiles_bin_.empty();
}

} // namespace porytiles2
