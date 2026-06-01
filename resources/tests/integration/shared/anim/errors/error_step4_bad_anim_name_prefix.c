// Error: "Could not extract animation name from '{}'"
// Array name doesn't match expected gTilesetAnims_General_ or sTilesetAnims_General_ prefix.

void InitTilesetAnim_General(void)
{
    sPrimaryTilesetAnimCallback = TilesetAnim_General;
}

static void TilesetAnim_General(u16 timer)
{
    if (timer % 16 == 0)
        QueueAnimTiles_General_Flower(timer / 16);
}

static void QueueAnimTiles_General_Flower(u16 timer)
{
    AppendTilesetAnimToBuffer(gWrongPrefix_Flower[timer], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(508)), 4 * TILE_SIZE_4BPP);
}
