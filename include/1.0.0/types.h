#ifndef PORYTILES_TYPES_H
#define PORYTILES_TYPES_H

#include <cstdint>
#include <compare>

struct RGBA32 {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;

    auto operator<=>(const RGBA32&) const = default;
};

#endif