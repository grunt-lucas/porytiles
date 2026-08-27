#ifndef GUARD_GLOBAL_FIELDMAP_H
#define GUARD_GLOBAL_FIELDMAP_H

// Same masks as the emerald fixture, so only the declaration differs between the two.
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF // Bits 0-7
#define METATILE_ATTR_LAYER_MASK    0xF000 // Bits 12-15

// struct Tileset is here, but it declares no metatileAttributes member at all. This is a different fact from "no
// struct Tileset", and a user chasing the difference needs to be told which one they have.
struct Tileset
{
    /*0x00*/ bool8 isCompressed;
    /*0x04*/ const u32 *tiles;
    /*0x0C*/ const u16 *metatiles;
    /*0x14*/ TilesetCB callback;
};

#endif // GUARD_GLOBAL_FIELDMAP_H
