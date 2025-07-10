#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/model/entities/RgbaMetatile.hpp"

namespace porytiles2 {

class PorytilesLayoutComponent {
  std::vector<RgbaMetatile> map_;
  std::vector<RgbaMetatile> border_;
};

} // namespace porytiles2
