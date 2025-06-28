#pragma once

#include <string>
#include <vector>

namespace porytiles {

class PorymapLayout {
  std::string name_;
  std::vector<int> map_;
  std::vector<int> border_;
  std::string primary_tileset_name_;
  std::string secondary_tileset_name_;
};

} // namespace porytiles
