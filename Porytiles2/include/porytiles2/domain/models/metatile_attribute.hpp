#pragma once

#include <cstddef>
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

class MetatileAttribute {
  public:
    MetatileAttribute() = default;

    MetatileAttribute(LayerType layerType, std::uint16_t behavior) : layer_type_{layerType}, behavior_{behavior} {}

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

  private:
    LayerType layer_type_;
    std::uint16_t behavior_;
};

} // namespace porytiles2