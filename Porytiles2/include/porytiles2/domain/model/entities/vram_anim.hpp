#pragma once

#include <vector>

namespace porytiles2 {

class VramAnim {
    std::vector<std::vector<std::uint8_t>> frames_;
    std::string name_;

  public:
    VramAnim() = default;
};

} // namespace porytiles2
