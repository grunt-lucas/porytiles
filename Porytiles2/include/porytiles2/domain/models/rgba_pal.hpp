#pragma once

#include <vector>

#include "porytiles2/domain/models/rgba32.hpp"

namespace porytiles2 {

namespace pal {

inline constexpr std::size_t max_size = 16;

inline constexpr std::size_t num_pals = 16;

} // namespace pal

class RgbaPal final {
  public:
    RgbaPal() = default;

    explicit RgbaPal(Rgba32 color);

    void add(Rgba32 color);

    void set(Rgba32 color, std::size_t index);

    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] const std::vector<Rgba32> &colors() const
    {
        return colors_;
    }

  private:
    std::vector<Rgba32> colors_;
};

} // namespace porytiles2
