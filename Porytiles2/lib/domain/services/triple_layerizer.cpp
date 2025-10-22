#include "porytiles2/domain/services/triple_layerizer.hpp"

#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

[[nodiscard]] ChainableResult<std::vector<TilemapEntry>> TripleLayerizer::triple_layerize(
    const std::vector<TilemapEntry> &entries,
    const std::vector<MetatileAttribute> &attributes,
    const Image<IndexPixel> &tiles,
    const std::array<RgbaPal, pal::num_pals> &pals)
{
    panic("TODO: implement");
}

} // namespace porytiles2
