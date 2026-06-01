// Error: "Expected token pattern containing '<tile_count_integer> * TILE_SIZE_4BPP'"
// Third argument is missing the TILE_SIZE_4BPP macro pattern.

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
    AppendTilesetAnimToBuffer(gTilesetAnims_General_Flower[timer], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(508)), 128);
}
