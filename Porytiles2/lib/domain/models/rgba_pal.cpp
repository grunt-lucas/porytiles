#include "porytiles2/domain/models/rgba_pal.hpp"

#include <string>

#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

RgbaPal::RgbaPal(Rgba32 color)
{
    // TODO: don't hardcode 16 here
    for (int i = 0; i < 16; i++) {
        add(color);
    }
}

void RgbaPal::add(Rgba32 color)
{
    colors_.push_back(color);
}

void RgbaPal::set(Rgba32 color, std::size_t index)
{
    if (index >= size()) {
        panic("index " + std::to_string(index) + " >= size " + std::to_string(size()));
    }
    colors_.at(index) = color;
}

[[nodiscard]] std::size_t RgbaPal::size() const
{
    return colors_.size();
}

} // namespace porytiles2
