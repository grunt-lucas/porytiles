#pragma once

#include <vector>

#include "porytiles2/domain/models/rgba32.hpp"

namespace porytiles2 {

class RgbaPal final {
  public:
    RgbaPal() = default;

    void add(Rgba32 color);

    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] const std::vector<Rgba32> &colors() const
    {
        return colors_;
    }

  private:
    std::vector<Rgba32> colors_;
};

} // namespace porytiles2
