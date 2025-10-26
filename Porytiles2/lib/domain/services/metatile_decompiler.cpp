#include "porytiles2/domain/services/metatile_decompiler.hpp"

#include <memory>
#include <vector>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/rgba_pal.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<std::vector<Metatile<Rgba32>>> MetatileDecompiler::decompile_metatiles(
    const std::vector<TilemapEntry> &entries,
    const Image<IndexPixel> &tiles,
    const std::array<RgbaPal, pal::num_pals> &pals)
{
    std::vector<Metatile<Rgba32>> decompiled;

    // Precondition: entry vector must be triple-layerized
    if (entries.size() % metatile::entries_per_metatile_triple != 0) {
        panic("entry vector size was not divisible 12");
    }

    // Iterate over groups of 12 TilemapEntries. For each group of 12, init a Metatile<Rgba32>. Then, starting from
    // bottom layer tile 0 through top layer tile 3, fill in the Metatile<Rgba32> by using the TilemapEntry values to
    // look up into the tiles and pals vectors.

    return decompiled;
}

} // namespace porytiles2
