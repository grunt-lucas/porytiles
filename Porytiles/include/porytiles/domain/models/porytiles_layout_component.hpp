#pragma once

#include <string>
#include <vector>

#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/rgba32.hpp"

namespace porytiles {

class PorytilesLayoutComponent {
    std::vector<Metatile<Rgba32>> map_;
    std::vector<Metatile<Rgba32>> border_;
};

} // namespace porytiles
