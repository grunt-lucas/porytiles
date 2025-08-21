#pragma once

namespace porytiles2 {

/**
 * @brief TODO: fill in doc string
 */
class TilemapEntry {
  public:
    TilemapEntry() = default;

    TilemapEntry(unsigned int tile_index, unsigned int pal_index, bool hflip, bool vflip)
        : tile_index_{tile_index}, pal_index_{pal_index}, hflip_{hflip}, vflip_{vflip}
    {
    }

    /**
     * @brief Checks if this TilemapEntry refers to the canonically transparent tile.
     *
     * @details
     * A TilemapEntry is "transparent" if it refers to the canonical transparent tile, which always has index 0. The
     * canonical transparent tile is a concept specific to Pokémon Generation III decomp projects. While not strictly
     * required by the Gen III engine, tile 0 in the vanilla tilesets is always transparent. Porytiles enforces this
     * explicitly.
     *
     * @param unused The extrinsic transparency value (unused for TilemapEntry)
     * @return True if this TilemapEntry refers to the transparent tile, false otherwise
     */
    [[nodiscard]] bool is_transparent(const TilemapEntry &unused) const;

    [[nodiscard]] unsigned int tile_index() const
    {
        return tile_index_;
    }

    [[nodiscard]] unsigned int pal_index() const
    {
        return pal_index_;
    }

    [[nodiscard]] bool hflip() const
    {
        return hflip_;
    }

    [[nodiscard]] bool vflip() const
    {
        return vflip_;
    }

  private:
    unsigned int tile_index_;
    unsigned int pal_index_;
    bool hflip_;
    bool vflip_;
};

} // namespace porytiles2
