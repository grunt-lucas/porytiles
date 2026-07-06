#include "global.h"
#include "fieldmap.h"

// Trimmed replica of pokeemerald-expansion's mask tables. sMetatileAttrMasks holds the FRLG masks, mixing raw hex with
// references to the _FRLG mask defines from global.fieldmap.h (which require cross-file symbol seeding to evaluate).
static const u32 sMetatileAttrMasks[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR]       = METATILE_ATTR_BEHAVIOR_MASK_FRLG, // Bits 0-8
    [METATILE_ATTRIBUTE_TERRAIN]        = 0x00003e00, // Bits 9-13
    [METATILE_ATTRIBUTE_2]              = 0x0003c000, // Bits 14-17
    [METATILE_ATTRIBUTE_3]              = 0x00fc0000, // Bits 18-23
    [METATILE_ATTRIBUTE_ENCOUNTER_TYPE] = 0x07000000, // Bits 24-26
    [METATILE_ATTRIBUTE_5]              = 0x18000000, // Bits 27-28
    [METATILE_ATTRIBUTE_LAYER_TYPE]     = METATILE_ATTR_LAYER_MASK_FRLG, // Bits 29-30
    [METATILE_ATTRIBUTE_7]              = 0x80000000  // Bit  31
};

// A decoy table with a different exact name. The exact-name rule must ignore it.
static const u32 sMetatileAttrMasksEmerald[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR]   = 0x000000ff,
    [METATILE_ATTRIBUTE_LAYER_TYPE] = 0x0000f000,
};
