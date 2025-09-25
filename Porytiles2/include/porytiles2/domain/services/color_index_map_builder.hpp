#pragma once

#include <map>
#include <vector>

#include "porytiles2/domain/model/normalized_tile.hpp"
#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

class ColorIndexMapBuilder {
  public:
    [[nodiscard]] std::map<Rgba32, unsigned int> build_map(const std::vector<NormalizedTile<Rgba32>> &tiles) const;
};

} // namespace porytiles2
