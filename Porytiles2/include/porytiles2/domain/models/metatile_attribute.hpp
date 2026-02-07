#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "porytiles2/domain/models/layer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

namespace attr {

/*
 * TODO: remove these hardcoded constants. fieldmap.c and global.fieldmap.h contain definitions for attribute shifts and
 * masks that could be used to infer these values.
 *
 * global.fieldmap.h (in pokeemerald) has:
 *   #define METATILE_ATTR_BEHAVIOR_MASK 0x00FF // Bits 0-7
 *   #define METATILE_ATTR_LAYER_MASK    0xF000 // Bits 12-15
 *   #define METATILE_ATTR_BEHAVIOR_SHIFT 0
 *   #define METATILE_ATTR_LAYER_SHIFT   12
 *
 * src/fieldmap.c (in pokefirered) has:
 * // Masks/shifts for metatile attributes
 * // This is the format of the data stored in each data/tilesets/x/x/metatile_attributes.bin file
 * static const u32 sMetatileAttrMasks[METATILE_ATTRIBUTE_COUNT] = {
 *    [METATILE_ATTRIBUTE_BEHAVIOR]       = 0x000001ff, // Bits 0-8
 *    [METATILE_ATTRIBUTE_TERRAIN]        = 0x00003e00, // Bits 9-13
 *    [METATILE_ATTRIBUTE_2]              = 0x0003c000, // Bits 14-17
 *    [METATILE_ATTRIBUTE_3]              = 0x00fc0000, // Bits 18-23
 *    [METATILE_ATTRIBUTE_ENCOUNTER_TYPE] = 0x07000000, // Bits 24-26
 *    [METATILE_ATTRIBUTE_5]              = 0x18000000, // Bits 27-28
 *    [METATILE_ATTRIBUTE_LAYER_TYPE]     = 0x60000000, // Bits 29-30
 *    [METATILE_ATTRIBUTE_7]              = 0x80000000  // Bit  31
 * };
 *
 * static const u8 sMetatileAttrShifts[METATILE_ATTRIBUTE_COUNT] = {
 *    [METATILE_ATTRIBUTE_BEHAVIOR]       = 0,
 *    [METATILE_ATTRIBUTE_TERRAIN]        = 9,
 *    [METATILE_ATTRIBUTE_2]              = 14,
 *    [METATILE_ATTRIBUTE_3]              = 18,
 *    [METATILE_ATTRIBUTE_ENCOUNTER_TYPE] = 24,
 *    [METATILE_ATTRIBUTE_5]              = 27,
 *    [METATILE_ATTRIBUTE_LAYER_TYPE]     = 29,
 *    [METATILE_ATTRIBUTE_7]              = 31
 * };
 */
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