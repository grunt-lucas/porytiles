#include "porytiles2/domain/model/color_set.hpp"

namespace porytiles2 {

bool ColorSet::test(std::size_t index) const
{
    return colors_.test(index);
}

void ColorSet::set(std::size_t index, bool value)
{
    colors_.set(index, value);
}

void ColorSet::reset(std::size_t index)
{
    colors_.reset(index);
}

} // namespace porytiles2
