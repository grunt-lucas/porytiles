#include "porytiles2/domain/services/layer_mode_converter.hpp"

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<std::vector<TilemapEntry>> LayerModeConverter::triple_layerize(
    const std::vector<TilemapEntry> &entries, const std::vector<MetatileAttribute> &attributes)
{
    panic("TODO: implement");
}

ChainableResult<std::vector<TilemapEntry>> LayerModeConverter::dual_layerize(
    const std::vector<TilemapEntry> &entries, const std::vector<MetatileAttribute> &attributes)
{
    panic("TODO: implement");
}

} // namespace porytiles2
