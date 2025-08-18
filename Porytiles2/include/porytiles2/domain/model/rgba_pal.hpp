#pragma once

#include <vector>

#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

class RgbaPal final {
  public:
    RgbaPal() = default;

    [[nodiscard]] std::size_t size() const {
        return colors_.size();
    }

    [[nodiscard]] const std::vector<Rgba32> &colors() const {
        return colors_;
    }

  private:
    std::vector<Rgba32> colors_;
};

} // namespace porytiles2
