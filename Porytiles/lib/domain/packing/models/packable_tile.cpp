#include "porytiles/domain/packing/models/packable_tile.hpp"

#include <cstddef>
#include <utility>

#include "porytiles/domain/models/color_set.hpp"

namespace porytiles {

PackableTile::PackableTile(HintId id, ColorSet color_set) : id_{std::move(id)}, color_set_{std::move(color_set)} {}

PackableTile::PackableTile(PrefilledPaletteId id, ColorSet color_set) : id_{id}, color_set_{std::move(color_set)} {}

PackableTile::PackableTile(RegularId id, ColorSet color_set) : id_{id}, color_set_{std::move(color_set)} {}

PackableTile::PackableTile(AnimId id, ColorSet color_set) : id_{id}, color_set_{std::move(color_set)} {}

PackableTile::PackableTile(PrimaryTileId id, ColorSet color_set) : id_{id}, color_set_{std::move(color_set)} {}

PackableTile::PackableTile(Id id, ColorSet color_set) : id_{std::move(id)}, color_set_{std::move(color_set)} {}

const std::string &PackableTile::hint_name() const
{
    return std::get<HintId>(id_).name;
}

std::size_t PackableTile::prefilled_index() const
{
    return std::get<PrefilledPaletteId>(id_).index();
}

std::size_t PackableTile::regular_index() const
{
    return std::get<RegularId>(id_).index;
}

std::size_t PackableTile::color_count() const
{
    return color_set_count(color_set_);
}

} // namespace porytiles
