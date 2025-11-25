#include "porytiles2/domain/services/primary_tileset_importer.hpp"

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetImporter::import(const Tileset &tileset) const
{
    // TODO: The resulting PorytilesTilesetComponent may be incomplete. E.g., the user may have specified
    // overrides; they will be present on disk. We don't want to clobber them when saving the decompiled
    // component. So we'll need to pull them from the original component and inject them into this one before
    // persisting.

    panic("TODO: implement");
}

} // namespace porytiles2
