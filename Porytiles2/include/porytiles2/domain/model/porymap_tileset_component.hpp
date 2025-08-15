#pragma once

#include <vector>

#include "porytiles2/domain/model/tilemap_entry.hpp"

namespace porytiles2 {

class PorymapTilesetComponent {
  public:
    PorymapTilesetComponent() = default;

    /**
     * @brief Add a tilemap entry to the end of the entries vector.
     *
     * @details
     * Moves the provided TilemapEntry into the entries vector.
     *
     * @param entry The TilemapEntry to move into the vector.
     */
    void push_back(TilemapEntry entry);

    [[nodiscard]] const std::vector<TilemapEntry> &metatiles_bin() const {
        return metatiles_bin_;
    }

    [[nodiscard]] bool is_empty() const;

  private:
    std::vector<TilemapEntry> metatiles_bin_;
};

} // namespace porytiles2
