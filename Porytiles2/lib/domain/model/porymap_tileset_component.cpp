#include "porytiles2/domain/model/porymap_tileset_component.hpp"

#include <utility>

#include "porytiles2/domain/model/vram_metatile.hpp"

namespace porytiles2 {

void PorymapTilesetComponent::push_back(VramMetatile metatile) {
    metatiles_.push_back(std::move(metatile));
}

bool PorymapTilesetComponent::is_empty() const {
    return metatiles_.empty();
}

} // namespace porytiles2
