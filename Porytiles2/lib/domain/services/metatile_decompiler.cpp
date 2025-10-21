#include "porytiles2/domain/services/metatile_decompiler.hpp"

#include <memory>
#include <vector>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/rgba_metatile.hpp"
#include "porytiles2/domain/models/rgba_pal.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<std::vector<RgbaMetatile>> MetatileDecompiler::decompile_metatiles(
    const std::vector<TilemapEntry> &entries,
    const std::vector<MetatileAttribute> &attributes,
    const Image<IndexPixel> &tiles,
    const std::array<RgbaPal, pal::num_pals> &pals)
{
    std::vector<RgbaMetatile> decompiled;

    // Precondition: entry vector must be triple-layerized
    if (entries.size() % 12 != 0) {
        // TODO: don't hardcode 12
        panic("entry vector size was not divisible 12");
    }
    if (entries.size() / 12 != attributes.size()) {
        panic("entry vector size (div 12) did not match attribute size");
    }

    return decompiled;
}

} // namespace porytiles2
