#include "porytiles2/domain/model/porytiles_tileset_component.hpp"

namespace porytiles2 {

bool PorytilesTilesetComponent::is_empty() const
{
    return bottom_->size() == 0 && middle_->size() == 0 && top_->size() == 0;
}

} // namespace porytiles2
