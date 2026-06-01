// Error: "Queue function '{}' has multiple AppendTilesetAnimToBuffer calls (VDests pattern not yet supported)."
// Queue function has two valid AppendTilesetAnimToBuffer calls.

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
    AppendTilesetAnimToBuffer(gTilesetAnims_General_Flower[timer], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(508)), 4 * TILE_SIZE_4BPP);
    AppendTilesetAnimToBuffer(gTilesetAnims_General_Flower[timer + 1], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(512)), 4 * TILE_SIZE_4BPP);
}
