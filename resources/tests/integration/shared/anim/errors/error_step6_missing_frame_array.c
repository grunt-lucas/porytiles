// Error: "Could not find frame array for animation '{}'"
// Everything is valid through step 5, but the frame pointer array for the animation is missing.

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
}

// No gTilesetAnims_General_Flower[] pointer array is defined.
// The parser will find the animation data but fail at step 6 because there is no matching frame array.
