#pragma once

#include <memory>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

class PorytilesTilesetComponent {
  public:
    PorytilesTilesetComponent()
        : bottom_{std::make_unique<Image<Rgba32>>()}, middle_{std::make_unique<Image<Rgba32>>()},
          top_{std::make_unique<Image<Rgba32>>()}
    {
    }

    [[nodiscard]] bool is_empty() const;

    [[nodiscard]] const Image<Rgba32> &bottom() const
    {
        return *bottom_;
    }

    void bottom(std::unique_ptr<Image<Rgba32>> bottom)
    {
        bottom_ = std::move(bottom);
    }

    [[nodiscard]] const Image<Rgba32> &middle() const
    {
        return *middle_;
    }

    void middle(std::unique_ptr<Image<Rgba32>> middle)
    {
        middle_ = std::move(middle);
    }

    [[nodiscard]] const Image<Rgba32> &top() const
    {
        return *top_;
    }

    void top(std::unique_ptr<Image<Rgba32>> top)
    {
        top_ = std::move(top);
    }

  private:
    std::unique_ptr<Image<Rgba32>> bottom_;
    std::unique_ptr<Image<Rgba32>> middle_;
    std::unique_ptr<Image<Rgba32>> top_;
};

} // namespace porytiles2
