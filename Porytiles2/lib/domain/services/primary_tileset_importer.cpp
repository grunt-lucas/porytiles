#include "porytiles2/domain/services/primary_tileset_importer.hpp"

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetImporter::import(const Tileset &tileset) const
{
    panic("TODO: implement");
}

} // namespace porytiles2
