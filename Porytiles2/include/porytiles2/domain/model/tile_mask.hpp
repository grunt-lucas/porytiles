#pragma once

#include <array>
#include <cstdint>

#include "porytiles2/domain/model/tile.hpp"

namespace porytiles2 {

class TileMask {
  public:
    TileMask() = default;

    explicit TileMask(const std::array<uint8_t, tile_side_length> &rows) : rows_{rows} {}

    // (lexicographic on rows array)
    auto operator<=>(const TileMask &) const = default;

    [[nodiscard]] TileMask get_flip(bool h, bool v) const;

    void set(int row, int col);

    void unset(int row, int col);

    [[nodiscard]] const std::array<uint8_t, 8> &rows() const
    {
        return rows_;
    }

  private:
    std::array<uint8_t, tile_side_length> rows_{};
};

} // namespace porytiles2

/**
 * @brief std::hash specialization for TileMask.
 *
 * @details
 * Provides a hash function for TileMask objects to enable their use in standard hash-based containers like
 * std::unordered_set and std::unordered_map. The hash is computed by combining each byte of the mask.
 */
template <>
struct std::hash<porytiles2::TileMask> {
    size_t operator()(const porytiles2::TileMask &tm) const noexcept
    {
        // Simple hash combining all 8 bytes
        size_t h = 0;
        for (const uint8_t byte : tm.rows()) {
            h = h * 31 + byte;
        }
        return h;
    }
};
