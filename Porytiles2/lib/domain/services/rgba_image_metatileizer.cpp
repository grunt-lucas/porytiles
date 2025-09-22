#include "porytiles2/domain/services/rgba_image_metatileizer.hpp"

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_metatile.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

[[nodiscard]] ChainableResult<std::vector<RgbaMetatile>>
RgbaImageMetatileizer::metatileize(const Image<Rgba32> &img) const
{
    return BasicError{"TODO: implement"};
}

} // namespace porytiles2
