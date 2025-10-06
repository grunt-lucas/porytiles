#include "porytiles2/domain/model/mask_pixels.hpp"

#include <cstdint>

#include "porytiles2/utilities/reverse_bits.hpp"

namespace porytiles2 {

MaskPixels MaskPixels::get_flip(bool h, bool v) const
{
    if (!h && !v) {
        return *this;
    }

    MaskPixels result;
    const int8_t v_inc = v ? -1 : 1;
    const int8_t v_start = v ? 7 : 0;

    for (int y = 0; y < 8; ++y) {
        const int ry = y * v_inc + v_start;
        result.rows_[y] = h ? reverse_bits(rows_[ry]) : rows_[ry];
    }
    return result;
}

void MaskPixels::set(int row, int col)
{
    rows_[row] |= (1 << (7 - col));
}

void MaskPixels::unset(int row, int col)
{
    rows_[row] &= ~(1 << (7 - col));
}

} // namespace porytiles2
