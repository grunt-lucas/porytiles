#pragma once

#include <algorithm>
#include <array>
#include <vector>

#include "porytiles2/domain/model/supports_transparency.hpp"
#include "porytiles2/domain/model/tile/pixel_tile.hpp"

namespace porytiles2 {

template <typename PixelType>
    requires SupportsTransparency<PixelType>
class CanonicalPixelTile : public PixelTile<PixelType> {
  public:
    virtual ~CanonicalPixelTile() = default;

    CanonicalPixelTile(const PixelTile<PixelType> &tile)
    {
        std::array<std::pair<bool, bool>, 4> flips = {{{false, false}, {false, true}, {true, false}, {true, true}}};

        std::vector<CanonicalPixelTile> candidates;
        candidates.reserve(4);

        for (const auto &[h, v] : flips) {
            candidates.emplace_back(flip(h, v), h, v);
        }

        auto min_candidate = *std::min_element(candidates.begin(), candidates.end());

        h_flip_ = min_candidate.h_flip();
        v_flip_ = min_candidate.v_flip();
        pix_ = min_candidate.pixels();
    }

    // Standard comparison (compares all fields in order)
    auto operator<=>(const IsoCanonicalTile &other) const = default;

    [[nodiscard]] bool h_flip() const
    {
        return h_flip_;
    }

    [[nodiscard]] bool v_flip() const
    {
        return v_flip_;
    }

  private:
    CanonicalPixelTile(const PixelTile<PixelType> &tile, bool h, bool v)
    {
        auto flipped_tile = tile.flip(h, v);
        pix_ = flipped_tile.pixels();
        h_flip_ = h;
        v_flip_ = v;
    }

    bool h_flip_;
    bool v_flip_;
};

} // namespace porytiles2
