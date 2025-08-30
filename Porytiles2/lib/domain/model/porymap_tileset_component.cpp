#include "porytiles2/domain/model/porymap_tileset_component.hpp"

#include <utility>

#include "fmt/format.h"

#include "porytiles2/domain/model/rgba_pal.hpp"
#include "porytiles2/domain/model/tilemap_entry.hpp"
#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

void PorymapTilesetComponent::push_back_tilemap_entry(TilemapEntry entry)
{
    metatiles_bin_.push_back(std::move(entry));
}

void PorymapTilesetComponent::set_pal(RgbaPal pal, int index)
{
    // TODO: don't hardcode 16 here
    if (index < 0 || index >= 16) {
        panic(fmt::format("invalid pal index {}: out of range", index));
    }
    pals_[index] = std::move(pal);
}

bool PorymapTilesetComponent::is_empty() const
{
    return metatiles_bin_.empty();
}

} // namespace porytiles2
