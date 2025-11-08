#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

namespace porytiles2 {

class PorytilesLayoutComponent {
    std::vector<Metatile<Rgba32>> map_;
    std::vector<Metatile<Rgba32>> border_;
};

} // namespace porytiles2
