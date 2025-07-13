#pragma once

#include "porytiles2/domain/model/valueobj/rgba32.hpp"
#include "porytiles2/domain/model/valueobj/tile.hpp"

namespace porytiles2 {

class RgbaTile final : public Tile<Rgba32> {
  public:
    RgbaTile() = default;

    [[nodiscard]] bool is_transparent(const Rgba32 &transparency) const override;

    [[nodiscard]] bool equals_bgr(const RgbaTile &other) const;
};

} // namespace porytiles2
