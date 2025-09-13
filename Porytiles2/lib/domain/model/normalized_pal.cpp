#include "porytiles2/domain/model/normalized_pal.hpp"

namespace porytiles2 {

void NormalizedPal::insert(const Rgba32 &color)
{
    colors_.insert(color);
}

[[nodiscard]] std::size_t NormalizedPal::size() const
{
    return colors_.size();
}

} // namespace porytiles2
