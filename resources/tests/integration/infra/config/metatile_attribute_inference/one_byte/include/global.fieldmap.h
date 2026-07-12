#ifndef GUARD_GLOBAL_FIELDMAP_H
#define GUARD_GLOBAL_FIELDMAP_H

// A 1-byte attribute layout (const u8 in metatiles.h): a single behavior field in the low nibble, no layer type.
// There is no vanilla 1-byte layer-type position, so no layer mask is declared and the layer type resolves to disabled.
#define METATILE_ATTR_BEHAVIOR_MASK 0x0F // Bits 0-3
#define METATILE_ATTR_BEHAVIOR_SHIFT 0

#endif // GUARD_GLOBAL_FIELDMAP_H
