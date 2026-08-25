#ifndef GUARD_GLOBAL_FIELDMAP_H
#define GUARD_GLOBAL_FIELDMAP_H

// Trimmed replica of pokefirered's attribute enum. The masks live in src/fieldmap.c, not here.
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
    METATILE_ATTRIBUTES_ALL = 255 // Special id to get the full attributes value; must not become a field.
};

// Value-name enums probed by the terrain and encounter_type fields. Note ENCOUNTER_TYPE must strip the _TYPE suffix
// to find the TILE_ENCOUNTER_ prefix.
enum
{
    TILE_TERRAIN_NORMAL,
    TILE_TERRAIN_GRASS,
    TILE_TERRAIN_WATER,
};

enum
{
    TILE_ENCOUNTER_NONE,
    TILE_ENCOUNTER_LAND,
    TILE_ENCOUNTER_WATER,
};

// Trimmed replica of pokefirered's struct Tileset: metatileAttributes is a u32 pointer (and sits after the callback),
// so the declared attribute element width is 4 bytes.
struct Tileset
{
    /*0x00*/ bool8 isCompressed:1;
    /*0x00*/ u8 swapPalettes:7;
    /*0x01*/ bool8 isSecondary;
    /*0x04*/ const u32 *tiles;
    /*0x08*/ const u16 (*palettes)[16];
    /*0x0c*/ const u16 *metatiles;
    /*0x10*/ TilesetCB callback;
    /*0x14*/ const u32 *metatileAttributes;
};

#endif // GUARD_GLOBAL_FIELDMAP_H
