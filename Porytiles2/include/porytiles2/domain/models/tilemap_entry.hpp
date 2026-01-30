#pragma once

#include <set>

namespace porytiles2 {

/**
 * @brief Represents a tilemap entry referencing a tile with palette and flip attributes.
 *
 * @details
 * TilemapEntry stores a reference to a tile (via tile_index) along with palette selection and flip flags. In Pokémon
 * Generation III decomp projects, tile index 0 is conventionally the transparent tile.
 *
 * @invariant Default-constructed TilemapEntry is transparent (satisfies SupportsTransparency design invariant). That
 * is, `TilemapEntry{}` produces an entry with tile_index=0, which refers to the canonical transparent tile.
 */
class TilemapEntry {
  public:
    TilemapEntry() : tile_index_{0}, pal_index_{0}, h_flip_{false}, v_flip_{false} {}

    TilemapEntry(std::size_t tile_index, std::size_t pal_index, bool hflip, bool vflip)
        : tile_index_{tile_index}, pal_index_{pal_index}, h_flip_{hflip}, v_flip_{vflip}
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
     * @return True if this TilemapEntry refers to the transparent tile, false otherwise
     */
    [[nodiscard]] bool is_transparent() const;

    [[nodiscard]] std::size_t tile_index() const
    {
        return tile_index_;
    }

    void tile_index(std::size_t tile_index)
    {
        tile_index_ = tile_index;
    }

    [[nodiscard]] std::size_t pal_index() const
    {
        return pal_index_;
    }

    void pal_index(std::size_t pal_index)
    {
        pal_index_ = pal_index;
    }

    [[nodiscard]] bool h_flip() const
    {
        return h_flip_;
    }

    void h_flip(bool hflip)
    {
        h_flip_ = hflip;
    }

    [[nodiscard]] bool v_flip() const
    {
        return v_flip_;
    }

    void v_flip(bool vflip)
    {
        v_flip_ = vflip;
    }

  private:
    std::size_t tile_index_;
    std::size_t pal_index_;
    bool h_flip_;
    bool v_flip_;
};

} // namespace porytiles2
