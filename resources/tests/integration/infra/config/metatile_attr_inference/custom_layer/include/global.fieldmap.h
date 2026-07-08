#ifndef GUARD_GLOBAL_FIELDMAP_H
#define GUARD_GLOBAL_FIELDMAP_H

// A ROM hack that moved the layer-type bits off the size convention (0xF000) to a custom position (0x0C00, bits 10-11).
// Inference must capture 0x0C00 rather than assuming the convention.
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF // Bits 0-7
#define METATILE_ATTR_LAYER_MASK    0x0C00 // Bits 10-11
#define METATILE_ATTR_BEHAVIOR_SHIFT 0
#define METATILE_ATTR_LAYER_SHIFT   10

#endif // GUARD_GLOBAL_FIELDMAP_H
