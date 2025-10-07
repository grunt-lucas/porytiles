#include "porytiles2/domain/model/tile/shape_mask.hpp"

#include <algorithm>
#include <cstdint>

#include "porytiles2/utilities/reverse_bits.hpp"

namespace porytiles2 {

ShapeMask ShapeMask::flip(bool h, bool v) const
{
    if (!h && !v) {
        return *this;
    }

    ShapeMask result;
    const int8_t v_inc = v ? -1 : 1;
    const int8_t v_start = v ? 7 : 0;

    for (int y = 0; y < 8; ++y) {
        const int ry = y * v_inc + v_start;
        result.rows_[y] = h ? reverse_bits(rows_[ry]) : rows_[ry];
    }
    return result;
}

void ShapeMask::set(int row, int col)
{
    rows_[row] |= (1 << (7 - col));
}

void ShapeMask::unset(int row, int col)
{
    rows_[row] &= ~(1 << (7 - col));
}

bool ShapeMask::is_transparent() const
{
    return std::ranges::all_of(rows_, [](uint8_t byte) { return byte == 0; });
}

} // namespace porytiles2
