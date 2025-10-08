#pragma once

#include <cstdint>
#include <tuple>

namespace porytiles2 {

namespace tile {

inline constexpr std::size_t side_length_pix = 8;
inline constexpr std::size_t size_pix = side_length_pix * side_length_pix;

} // namespace tile

namespace metatile {

inline constexpr std::size_t tiles_per_side = 2;
inline constexpr std::size_t tiles_per_metatile = tiles_per_side * tiles_per_side;
inline constexpr std::size_t side_length_pix = tiles_per_side * tile::side_length_pix;

enum class Layer : std::uint8_t { bottom = 0, middle = 1, top = 2 };

enum class Subtile : std::uint8_t { northwest = 0, northeast = 1, southwest = 2, southeast = 3 };

[[nodiscard]] inline std::tuple<std::size_t, Layer, Subtile> compute_metatile(std::size_t tile_index)
{
    // Metatile has 12 subtiles: 4 per layer × 3 layers
    constexpr std::size_t total_tiles_per_metatile = tiles_per_metatile * 3;

    const std::size_t metatile_index = tile_index / total_tiles_per_metatile;
    const std::size_t local_index = tile_index % total_tiles_per_metatile;
    const auto layer = static_cast<Layer>(local_index / tiles_per_metatile);
    const auto subtile = static_cast<Subtile>(local_index % tiles_per_metatile);

    return {metatile_index, layer, subtile};
}

} // namespace metatile

} // namespace porytiles2
