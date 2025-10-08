#include "porytiles2/domain/models/rgba_pal.hpp"

namespace porytiles2 {

void RgbaPal::add(Rgba32 color)
{
    colors_.push_back(color);
}

[[nodiscard]] std::size_t RgbaPal::size() const
{
    return colors_.size();
}

} // namespace porytiles2
