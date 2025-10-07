#pragma once

#include <variant>

namespace porytiles2 {

namespace tile {

inline constexpr std::size_t side_length_pix = 8;
inline constexpr std::size_t size_pix = side_length_pix * side_length_pix;

} // namespace tile

namespace metatile {

inline constexpr std::size_t tiles_per_side = 2;
inline constexpr std::size_t tiles_per_metatile = tiles_per_side * tiles_per_side;
inline constexpr std::size_t side_length_pix = tiles_per_side * tile::side_length_pix;

} // namespace metatile

} // namespace porytiles2