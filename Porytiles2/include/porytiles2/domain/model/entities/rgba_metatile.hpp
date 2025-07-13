#pragma once

#include <array>

#include "porytiles2/domain/model/valueobj/rgba_tile.hpp"

namespace porytiles2 {

class RgbaMetatile {
  public:
    RgbaMetatile() = default;

    /**
     * @brief Get a constant reference to a tile from the bottom layer.
     *
     * @details
     * Retrieves the RgbaTile at the specified index in the bottom layer array.
     *
     * @param i The index into the bottom layer array (must be 0-3).
     * @return Constant reference to the RgbaTile at the specified index.
     */
    [[nodiscard]] const RgbaTile &bottom(std::size_t i) const;

    /**
     * @brief Set a tile in the bottom layer.
     *
     * @details
     * Moves the provided RgbaTile into the specified index of the bottom layer array.
     *
     * @param i The index into the bottom layer array (must be 0-3).
     * @param tile The RgbaTile to move into the array.
     */
    void set_bottom(std::size_t i, RgbaTile tile);

    /**
     * @brief Get a constant reference to a tile from the middle layer.
     *
     * @details
     * Retrieves the RgbaTile at the specified index in the middle layer array.
     *
     * @param i The index into the middle layer array (must be 0-3).
     * @return Constant reference to the RgbaTile at the specified index.
     */
    [[nodiscard]] const RgbaTile &middle(std::size_t i) const;

    /**
     * @brief Set a tile in the middle layer.
     *
     * @details
     * Moves the provided RgbaTile into the specified index of the middle layer array.
     *
     * @param i The index into the middle layer array (must be 0-3).
     * @param tile The RgbaTile to move into the array.
     */
    void set_middle(std::size_t i, RgbaTile tile);

    /**
     * @brief Get a constant reference to a tile from the top layer.
     *
     * @details
     * Retrieves the RgbaTile at the specified index in the top layer array.
     *
     * @param i The index into the top layer array (must be 0-3).
     * @return Constant reference to the RgbaTile at the specified index.
     */
    [[nodiscard]] const RgbaTile &top(std::size_t i) const;

    /**
     * @brief Set a tile in the top layer.
     *
     * @details
     * Moves the provided RgbaTile into the specified index of the top layer array.
     *
     * @param i The index into the top layer array (must be 0-3).
     * @param tile The RgbaTile to move into the array.
     */
    void set_top(std::size_t i, RgbaTile tile);

  private:
    std::array<RgbaTile, 4> bottom_;
    std::array<RgbaTile, 4> middle_;
    std::array<RgbaTile, 4> top_;
};

} // namespace porytiles2
