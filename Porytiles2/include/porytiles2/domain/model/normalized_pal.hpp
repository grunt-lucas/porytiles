#pragma once

#include <set>

#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

/**
 * @brief A palette that inserts colors in a consistent sorted order.
 */
class NormalizedPal {
  public:
    NormalizedPal() = default;

    void insert(const Rgba32 &color);

    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] const std::set<Rgba32> &colors() const
    {
        return colors_;
    }

  private:
    std::set<Rgba32> colors_;
};

} // namespace porytiles2
