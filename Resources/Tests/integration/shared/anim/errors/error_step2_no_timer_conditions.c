// Error: "No timer conditions found in driver function '{}'"
// Driver function exists but has no `timer % X == Y` conditions.

void InitTilesetAnim_General(void)
{
    sPrimaryTilesetAnimCallback = TilesetAnim_General;
}

static void TilesetAnim_General(u16 timer)
{
    int x = 0;
}
