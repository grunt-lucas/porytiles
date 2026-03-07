// Error: "Found multiple callback functions matching '{}'"
// Two functions with the same Init callback name triggers the multiple-match error.

void InitTilesetAnim_General(void)
{
    sPrimaryTilesetAnimCallback = TilesetAnim_General;
}

void InitTilesetAnim_General(void)
{
    sPrimaryTilesetAnimCallback = TilesetAnim_General;
}
