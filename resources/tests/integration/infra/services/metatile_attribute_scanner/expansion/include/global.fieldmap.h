#ifndef GUARD_GLOBAL_FIELDMAP_H
#define GUARD_GLOBAL_FIELDMAP_H

// Trimmed replica of pokeemerald-expansion's dual-layout attribute defines. Bare masks are the emerald-side primary
// masks; the _FRLG variants are the alternate layout.
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF // Bits 0-7
#define METATILE_ATTR_LAYER_MASK    0xF000 // Bits 12-15

#define METATILE_ATTR_BEHAVIOR_MASK_FRLG  0x000001ff // Bits 0-8
#define METATILE_ATTR_LAYER_MASK_FRLG     0x60000000 // Bits 29-30

// An unresolvable backslash-continuation define, as in expansion. The tolerant scan records the name and continues.
#define FOLLOWER_INVISIBLE_FLAGS (PLAYER_AVATAR_STATE_A | \
                                  PLAYER_AVATAR_STATE_B)

enum
{
    METATILE_ATTRIBUTE_BEHAVIOR,
    METATILE_ATTRIBUTE_TERRAIN,
    METATILE_ATTRIBUTE_2,
    METATILE_ATTRIBUTE_3,
    METATILE_ATTRIBUTE_ENCOUNTER_TYPE,
    METATILE_ATTRIBUTE_5,
    METATILE_ATTRIBUTE_LAYER_TYPE,
    METATILE_ATTRIBUTE_7,
    METATILE_ATTRIBUTE_COUNT,
};

enum
{
    TILE_TERRAIN_NORMAL,
    TILE_TERRAIN_GRASS,
};

enum
{
    TILE_ENCOUNTER_NONE,
    TILE_ENCOUNTER_LAND,
};

#endif // GUARD_GLOBAL_FIELDMAP_H
