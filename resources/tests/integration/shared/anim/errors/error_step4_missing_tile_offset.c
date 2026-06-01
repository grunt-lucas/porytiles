// Error: "Expected token pattern containing 'TILE_OFFSET_4BPP(<tile_offset_integer>)'"
// Second argument is missing the TILE_OFFSET_4BPP macro pattern.

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
    AppendTilesetAnimToBuffer(gTilesetAnims_General_Flower[timer], (u16 *)(BG_VRAM + 508), 4 * TILE_SIZE_4BPP);
}
