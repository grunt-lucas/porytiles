#ifndef GUARD_FIELDMAP_ENUMS_H
#define GUARD_FIELDMAP_ENUMS_H

/*
 * A complex #define that references other tokens. It is only parseable as an
 * enum-format header; enums_only providers must never attempt to evaluate it.
 */
#define METATILE_ID(tileset, name) (METATILE_##tileset##_##name)

enum {
  TILE_ENCOUNTER_NONE,
  TILE_ENCOUNTER_LAND,
  TILE_ENCOUNTER_WATER,
};

enum {
  TILE_TERRAIN_NORMAL,
  TILE_TERRAIN_GRASS,
  TILE_TERRAIN_WATER,
  TILE_TERRAIN_WATERFALL,
};

#endif // GUARD_FIELDMAP_ENUMS_H
