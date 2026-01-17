#include "porytiles2/domain/services/primary_tileset_creator.hpp"

#include <memory>

#include "porytiles2/domain/models/porytiles_tileset_component.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<PorytilesTilesetComponent>>
PrimaryTilesetCreator::create_porytiles_component(const std::string & /*tileset_name*/) const
{
    // Create a blank PorytilesTilesetComponent with default empty state
    // The default constructor initializes:
    // - Empty layer images (0x0 dimensions)
    // - No metatile attributes
    // - No palettes
    // - No animations
    return std::make_unique<PorytilesTilesetComponent>();
}

} // namespace porytiles2
