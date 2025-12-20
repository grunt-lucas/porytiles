#ifndef GUARD_GENERATED_ANIM_CODE_General_H
#define GUARD_GENERATED_ANIM_CODE_General_H

// Ensure INCBIN_U16 is available
#ifndef INCBIN_U16
#include "gba/defines.h"
#endif

// ============================================
// Frame Data (INCBIN statements)
// ============================================

const u16 gTilesetAnims_PorytilesManaged_General_Flower_Frame0[] = INCBIN_U16("data/tilesets/primary/general/include/anim/flower/0.4bpp");
const u16 gTilesetAnims_PorytilesManaged_General_Flower_Frame1[] = INCBIN_U16("data/tilesets/primary/general/include/anim/flower/1.4bpp");
const u16 gTilesetAnims_PorytilesManaged_General_Flower_Frame2[] = INCBIN_U16("data/tilesets/primary/general/include/anim/flower/2.4bpp");

const u16 gTilesetAnims_PorytilesManaged_General_Water_Frame0[] = INCBIN_U16("data/tilesets/primary/general/include/anim/water/0.4bpp");
const u16 gTilesetAnims_PorytilesManaged_General_Water_Frame1[] = INCBIN_U16("data/tilesets/primary/general/include/anim/water/1.4bpp");
const u16 gTilesetAnims_PorytilesManaged_General_Water_Frame2[] = INCBIN_U16("data/tilesets/primary/general/include/anim/water/2.4bpp");
const u16 gTilesetAnims_PorytilesManaged_General_Water_Frame3[] = INCBIN_U16("data/tilesets/primary/general/include/anim/water/3.4bpp");
const u16 gTilesetAnims_PorytilesManaged_General_Water_Frame4[] = INCBIN_U16("data/tilesets/primary/general/include/anim/water/4.4bpp");

// ============================================
// Frame Pointer Arrays
// ============================================

const u16 *const gTilesetAnims_PorytilesManaged_General_Flower[] = {
    gTilesetAnims_PorytilesManaged_General_Flower_Frame0,
    gTilesetAnims_PorytilesManaged_General_Flower_Frame1,
    gTilesetAnims_PorytilesManaged_General_Flower_Frame0,
    gTilesetAnims_PorytilesManaged_General_Flower_Frame2};

const u16 *const gTilesetAnims_PorytilesManaged_General_Water[] = {
    gTilesetAnims_PorytilesManaged_General_Water_Frame0,
    gTilesetAnims_PorytilesManaged_General_Water_Frame1,
    gTilesetAnims_PorytilesManaged_General_Water_Frame2,
    gTilesetAnims_PorytilesManaged_General_Water_Frame3,
    gTilesetAnims_PorytilesManaged_General_Water_Frame4};

// ============================================
// Queue Functions
// ============================================

static void QueueAnimTiles_PorytilesManaged_General_Flower(u16 timer) {
  u16 i = timer % ARRAY_COUNT(gTilesetAnims_PorytilesManaged_General_Flower);
  AppendTilesetAnimToBuffer(gTilesetAnims_PorytilesManaged_General_Flower[i], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(12)), 4 * TILE_SIZE_4BPP);
}

static void QueueAnimTiles_PorytilesManaged_General_Water(u16 timer) {
  u16 i = timer % ARRAY_COUNT(gTilesetAnims_PorytilesManaged_General_Water);
  AppendTilesetAnimToBuffer(gTilesetAnims_PorytilesManaged_General_Water[i], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(432)), 30 * TILE_SIZE_4BPP);
}

// ============================================
// Driver Function
// ============================================

static void TilesetAnim_PorytilesManaged_General(u16 timer) {
  if (timer % 16 == 0) {
    QueueAnimTiles_PorytilesManaged_General_Flower(timer / 16);
  }
  if (timer % 16 == 1) {
    QueueAnimTiles_PorytilesManaged_General_Water(timer / 16);
  }
}

// ============================================
// Init Function
// ============================================

void InitTilesetAnim_PorytilesManaged_General(void) {
  sPrimaryTilesetAnimCounter = 0;
  sPrimaryTilesetAnimCounterMax = 256;
  sPrimaryTilesetAnimCallback = TilesetAnim_PorytilesManaged_General;
}

#endif // GUARD_GENERATED_ANIM_CODE_General_H
