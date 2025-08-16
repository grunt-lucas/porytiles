#pragma once

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

class PorytilesTilesetComponent {
  public:
    PorytilesTilesetComponent() = default;

    [[nodiscard]] bool is_empty() const;

  private:
    Image<Rgba32> bottom_;
    Image<Rgba32> middle_;
    Image<Rgba32> top_;
};

} // namespace porytiles2
