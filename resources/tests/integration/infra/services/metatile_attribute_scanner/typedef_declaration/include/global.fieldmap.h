#ifndef GUARD_GLOBAL_FIELDMAP_H
#define GUARD_GLOBAL_FIELDMAP_H

// Same masks as the emerald fixture, so only the declaration differs between the two.
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF // Bits 0-7
#define METATILE_ATTR_LAYER_MASK    0xF000 // Bits 12-15

// A fork that renamed the attribute element type behind a typedef. The declaration is perfectly valid C and the
// parser reads it without complaint, but the name says nothing about the width, so the scan records the declarator
// as written and leaves the ruling to the domain.
typedef u16 MetatileAttr;

struct Tileset
{
    /*0x00*/ bool8 isCompressed;
    /*0x0C*/ const u16 *metatiles;
    /*0x10*/ const MetatileAttr *metatileAttributes;
    /*0x14*/ TilesetCB callback;
};

#endif // GUARD_GLOBAL_FIELDMAP_H
