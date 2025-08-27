#pragma once

#include <memory>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

class PorytilesTilesetComponent {
  public:
    PorytilesTilesetComponent() = default;

    [[nodiscard]] bool is_empty() const;

    [[nodiscard]] const Image<Rgba32> &bottom() const
    {
        return bottom_;
    }

    void bottom(const Image<Rgba32> &bottom)
    {
        bottom_ = bottom;
    }

    [[nodiscard]] const Image<Rgba32> &middle() const
    {
        return middle_;
    }

    void middle(const Image<Rgba32> &middle)
    {
        middle_ = middle;
    }

    [[nodiscard]] const Image<Rgba32> &top() const
    {
        return top_;
    }

    void top(const Image<Rgba32> &top)
    {
        top_ = top;
    }

  private:
    Image<Rgba32> bottom_;
    Image<Rgba32> middle_;
    Image<Rgba32> top_;
};

} // namespace porytiles2
