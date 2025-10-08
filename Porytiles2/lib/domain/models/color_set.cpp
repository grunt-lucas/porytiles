#include "porytiles2/domain/models/color_set.hpp"

namespace porytiles2 {

bool ColorSet::test(ColorIndex index) const
{
    return colors_.test(index.index());
}

void ColorSet::set(ColorIndex index, bool value)
{
    colors_.set(index.index(), value);
}

void ColorSet::reset(ColorIndex index)
{
    colors_.reset(index.index());
}

} // namespace porytiles2
