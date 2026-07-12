#ifndef GUARD_GLOBAL_FIELDMAP_H
#define GUARD_GLOBAL_FIELDMAP_H

// Trimmed replica of stock pokeemerald's attribute mask defines. Behavior and layer masks only, with the layer
// suffix spelled LAYER (not LAYER_TYPE) to exercise suffix normalization.
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF // Bits 0-7
#define METATILE_ATTR_LAYER_MASK    0xF000 // Bits 12-15
#define METATILE_ATTR_BEHAVIOR_SHIFT 0
#define METATILE_ATTR_LAYER_SHIFT   12

// A backslash-continuation define that references cross-header symbols the scanner cannot resolve. The tolerant scan
// must record its name and move on rather than aborting.
#define FOLLOWER_INVISIBLE_FLAGS (FOLLOWER_FLAG_A | \
                                  FOLLOWER_FLAG_B)

#endif // GUARD_GLOBAL_FIELDMAP_H
