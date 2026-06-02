// Error: "AppendTilesetAnimToBuffer call in '{}' has fewer than 3 arguments."
// AppendTilesetAnimToBuffer is called with only 2 arguments instead of 3.

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
    AppendTilesetAnimToBuffer(gTilesetAnims_General_Flower[timer], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(508)));
}
