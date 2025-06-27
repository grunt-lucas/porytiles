#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/entities/RgbaMetatile.hpp"

namespace porytiles {

class PorytilesLayout {
    std::string name_;
    std::vector<RgbaMetatile> metatiles_;
};

} // namespace porytiles
