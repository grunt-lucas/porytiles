#include "fieldmap.h"

// The behavior mask has offset 0, but the shift table claims 4. The scanner records both raw facts; inference
// records the disagreement as a shift_vs_mask conflict for the reconciler to rule on.
static const u32 sMetatileAttrMasks[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR] = 0x00ff,
};

static const u8 sMetatileAttrShifts[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR] = 4,
};
