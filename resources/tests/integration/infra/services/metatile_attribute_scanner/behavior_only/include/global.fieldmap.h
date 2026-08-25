#ifndef GUARD_GLOBAL_FIELDMAP_H
#define GUARD_GLOBAL_FIELDMAP_H

// A stock-shaped project declaring only the attribute enum, with no masks anywhere: inference synthesizes the
// two-byte behavior-only layout, and the synthesized set must not drive size inference.
enum
{
    METATILE_ATTRIBUTE_BEHAVIOR,
    METATILE_ATTRIBUTE_LAYER_TYPE,
    METATILE_ATTRIBUTE_COUNT,
};

#endif // GUARD_GLOBAL_FIELDMAP_H
