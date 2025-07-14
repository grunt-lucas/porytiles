#include "porytiles2/domain/model/aggregates/components/porytiles_tileset_component.hpp"

#include <utility>

#include "porytiles2/domain/model/entities/rgba_metatile.hpp"

namespace porytiles2 {

void PorytilesTilesetComponent::push_back(RgbaMetatile metatile) {
    metatiles_.push_back(std::move(metatile));
}

} // namespace porytiles2
