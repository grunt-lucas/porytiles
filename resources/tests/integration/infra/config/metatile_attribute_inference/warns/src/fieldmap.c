#include "fieldmap.h"

// The behavior mask has offset 0, but the shift table claims 4. Inference must keep the mask and emit a warning.
static const u32 sMetatileAttrMasks[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR] = 0x00ff,
};

static const u8 sMetatileAttrShifts[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR] = 4,
};
