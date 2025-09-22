#pragma once

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_metatile.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

class RgbaImageMetatileizer {
  public:
    [[nodiscard]] ChainableResult<std::vector<RgbaMetatile>> metatileize(const Image<Rgba32> &img) const;
};

} // namespace porytiles2
