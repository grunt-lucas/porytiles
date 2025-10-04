#include "porytiles2/domain/model/ordered_pal.hpp"

#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

void OrderedPal::insert(const Rgba32 &color)
{
    colors_.insert(color);
}

[[nodiscard]] std::size_t OrderedPal::size() const
{
    return colors_.size();
}

} // namespace porytiles2
