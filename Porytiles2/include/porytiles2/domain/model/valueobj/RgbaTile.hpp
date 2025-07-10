#pragma once

#include "porytiles2/domain/model/valueobj/Rgba32.hpp"
#include "porytiles2/domain/model/valueobj/Tile.hpp"

namespace porytiles {

class RgbaTile final : public Tile<Rgba32> {
public:
  explicit RgbaTile() : Tile{} {}

  [[nodiscard]] bool is_transparent(const Rgba32 &transparency) const override;

  [[nodiscard]] bool equals_bgr(const RgbaTile &other) const;
};

} // namespace porytiles
