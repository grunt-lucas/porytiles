#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/model/entities/RgbaAnim.hpp"
#include "porytiles2/domain/model/entities/RgbaMetatile.hpp"

namespace porytiles2 {

class PorytilesTilesetComponent {
  std::vector<RgbaMetatile> metatiles_;
  std::vector<RgbaAnim> anims_;
};

} // namespace porytiles2
