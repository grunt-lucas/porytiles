#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "porytiles2/domain/models/layer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

namespace attr {

constexpr std::size_t bytes_per_attr_emerald = 2;
constexpr std::size_t bytes_per_attr_firered = 4;

} // namespace attr

/**
 * @brief Represents the attributes of a single metatile.
 *
 * @details
 * Emerald uses a 2-byte format encoding behavior (bits 0-7) and layer type (bits 12-15).
 * FireRed uses a 4-byte format encoding behavior (bits 0-8), terrain (bits 9-13),
 * attribute_2 (bits 14-17), attribute_3 (bits 18-23), encounter_type (bits 24-26),
 * attribute_5 (bits 27-28), layer_type (bits 29-30), and attribute_7 (bit 31).
 *
 * The FireRed-specific fields default to zero, so existing Emerald code is unaffected.
 */
class MetatileAttribute {
  public:
    MetatileAttribute() = default;

    /**
     * @brief Constructs an Emerald-format metatile attribute.
     *
     * @details
     * FireRed-specific fields are initialized to zero.
     */
    MetatileAttribute(LayerType layer_type, std::uint16_t behavior) : layer_type_{layer_type}, behavior_{behavior} {}

    /**
     * @brief Constructs a FireRed-format metatile attribute with all fields.
     */
    MetatileAttribute(
        LayerType layer_type,
        std::uint16_t behavior,
        std::uint8_t terrain,
        std::uint8_t encounter_type,
        std::uint8_t attribute_2,
        std::uint8_t attribute_3,
        std::uint8_t attribute_5,
        bool attribute_7)
        : layer_type_{layer_type}, behavior_{behavior}, terrain_{terrain}, encounter_type_{encounter_type},
          attribute_2_{attribute_2}, attribute_3_{attribute_3}, attribute_5_{attribute_5}, attribute_7_{attribute_7}
    {
    }

    [[nodiscard]] LayerType layer_type() const
    {
        return layer_type_;
    }

    void layer_type(LayerType layer_type)
    {
        layer_type_ = layer_type;
    }

    [[nodiscard]] std::uint16_t behavior() const
    {
        return behavior_;
    }

    void behavior(std::uint16_t behavior)
    {
        behavior_ = behavior;
    }

    [[nodiscard]] std::uint8_t terrain() const
    {
        return terrain_;
    }

    void terrain(std::uint8_t terrain)
    {
        terrain_ = terrain;
    }

    [[nodiscard]] std::uint8_t encounter_type() const
    {
        return encounter_type_;
    }

    void encounter_type(std::uint8_t encounter_type)
    {
        encounter_type_ = encounter_type;
    }

    [[nodiscard]] std::uint8_t attribute_2() const
    {
        return attribute_2_;
    }

    void attribute_2(std::uint8_t attribute_2)
    {
        attribute_2_ = attribute_2;
    }

    [[nodiscard]] std::uint8_t attribute_3() const
    {
        return attribute_3_;
    }

    void attribute_3(std::uint8_t attribute_3)
    {
        attribute_3_ = attribute_3;
    }

    [[nodiscard]] std::uint8_t attribute_5() const
    {
        return attribute_5_;
    }

    void attribute_5(std::uint8_t attribute_5)
    {
        attribute_5_ = attribute_5;
    }

    [[nodiscard]] bool attribute_7() const
    {
        return attribute_7_;
    }

    void attribute_7(bool attribute_7)
    {
        attribute_7_ = attribute_7;
    }

  private:
    LayerType layer_type_{};
    std::uint16_t behavior_{};
    std::uint8_t terrain_{};
    std::uint8_t encounter_type_{};
    std::uint8_t attribute_2_{};
    std::uint8_t attribute_3_{};
    std::uint8_t attribute_5_{};
    bool attribute_7_{};
};

} // namespace porytiles2