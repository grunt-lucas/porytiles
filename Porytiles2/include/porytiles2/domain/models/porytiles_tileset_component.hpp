#pragma once

#include <array>
#include <optional>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

class PorytilesTilesetComponent {
  public:
    PorytilesTilesetComponent() = default;

    void set_pal(std::size_t pal_index, Palette<Rgba32, pal::max_size> pal);

    [[nodiscard]] const std::optional<Palette<Rgba32, pal::max_size>> &pal_at(std::size_t pal_index) const;

    [[nodiscard]] bool is_empty() const;

    [[nodiscard]] ChainableResult<LayerMode> detect_layer_mode(const Rgba32 &extrinsic) const;

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

    [[nodiscard]] const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &pals() const
    {
        return pals_;
    }

  private:
    Image<Rgba32> bottom_;
    Image<Rgba32> middle_;
    Image<Rgba32> top_;
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> pals_;
};

} // namespace porytiles2
