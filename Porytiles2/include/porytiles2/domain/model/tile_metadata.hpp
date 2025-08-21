#pragma once

#include <variant>

namespace porytiles2 {

enum class TileType { vram, free, layered, anim, primer, override };

class FreeMetadata {
    std::size_t tile_index_;

  public:
    explicit FreeMetadata() : tile_index_{0} {}

    [[nodiscard]] std::size_t tile_index() const
    {
        return tile_index_;
    }

    void set_tile_index(const std::size_t tile_index)
    {
        tile_index_ = tile_index;
    }
};

class LayeredMetadata {
    std::size_t metatile_index_;

  public:
    explicit LayeredMetadata() : metatile_index_{0} {}

    [[nodiscard]] std::size_t metatile_index() const
    {
        return metatile_index_;
    }

    void set_metatile_index(const std::size_t metatile_index)
    {
        metatile_index_ = metatile_index;
    }
};

using TileMetadata = std::variant<std::monostate, FreeMetadata, LayeredMetadata>;

} // namespace porytiles2