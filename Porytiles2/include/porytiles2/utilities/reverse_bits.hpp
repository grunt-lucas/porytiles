#pragma once

#include <cstdint>

namespace porytiles2 {

/**
 * @brief Reverses the bits in a byte.
 *
 * @details
 * This function reverses the order of all 8 bits in a byte. For example, the byte 0b10110010 would be reversed to
 * 0b01001101.
 *
 * @param b The byte to reverse
 * @return The byte with its bits reversed
 */
constexpr uint8_t reverse_bits(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

} // namespace porytiles2
