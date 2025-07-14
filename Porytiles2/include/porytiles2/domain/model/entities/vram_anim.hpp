#pragma once

#include <vector>

namespace porytiles2 {

class VramAnim {
  public:
    VramAnim() = default;

  private:
    std::vector<std::vector<std::uint8_t>> frames_;
    std::string name_;
};

} // namespace porytiles2
