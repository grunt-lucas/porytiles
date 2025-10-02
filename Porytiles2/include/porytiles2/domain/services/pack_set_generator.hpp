#pragma once

#include "gsl/pointers"

#include "porytiles2/domain/model/normalized_tile.hpp"
#include "porytiles2/domain/model/pack_set.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/services/color_set_builder.hpp"

namespace porytiles2 {

class PackSetGenerator {
  public:
    explicit PackSetGenerator(gsl::not_null<ColorSetBuilder *> color_set_builder)
        : color_set_builder_{color_set_builder}
    {
    }

    [[nodiscard]] std::vector<PackSet> generate(
        const std::vector<NormalizedTile<Rgba32>> &norm_tiles,
        const std::map<Rgba32, unsigned int> &color_index_map) const;

  private:
    ColorSetBuilder *color_set_builder_;
};

} // namespace porytiles2
