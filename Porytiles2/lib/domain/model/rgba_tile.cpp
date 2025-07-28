#include "porytiles2/domain/model/rgba_tile.hpp"

#include <ranges>

#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

bool RgbaTile::is_transparent(const Rgba32 &extrinsic) const {
    return std::ranges::all_of(
        pix(), [=](const auto &pixel) { return pixel == extrinsic || pixel.alpha() == Rgba32::alpha_transparent; });
}

bool RgbaTile::equals_as_bgr(const RgbaTile &other) const {
    panic("TODO: unimplemented");
}

} // namespace porytiles2
