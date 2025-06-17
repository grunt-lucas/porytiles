#pragma once

#include <porytiles2/colors/rgba32.hpp>
#include <porytiles2/tiles/tile.hpp>

namespace porytiles {

class RgbaTile final : public Tile<Rgba32> {
  public:
    explicit RgbaTile() : Tile{} {}

    [[nodiscard]] bool IsTransparent(const Rgba32 &transparency) const override;

    [[nodiscard]] bool EqualsBgr(const RgbaTile &other) const;
};

} // namespace porytiles
