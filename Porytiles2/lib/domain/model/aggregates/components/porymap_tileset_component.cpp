#include "porytiles2/domain/model/aggregates/components/porymap_tileset_component.hpp"

#include <utility>

#include "porytiles2/domain/model/entities/vram_metatile.hpp"

namespace porytiles2 {

void PorymapTilesetComponent::push_back(VramMetatile metatile) {
    metatiles_.push_back(std::move(metatile));
}

} // namespace porytiles2
