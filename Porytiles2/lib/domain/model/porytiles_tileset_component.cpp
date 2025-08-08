#include "porytiles2/domain/model/porytiles_tileset_component.hpp"

#include <utility>

#include "porytiles2/domain/model/rgba_metatile.hpp"

namespace porytiles2 {

void PorytilesTilesetComponent::push_back(RgbaMetatile metatile) {
    metatiles_.push_back(std::move(metatile));
}

bool PorytilesTilesetComponent::is_empty() const {
    return metatiles_.empty();
}

} // namespace porytiles2
