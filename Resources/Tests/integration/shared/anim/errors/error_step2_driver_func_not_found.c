// Error: "Driver function '{}' not found in file."
// Callback assigns to a driver function that is never defined in the file.

void InitTilesetAnim_General(void)
{
    sPrimaryTilesetAnimCallback = TilesetAnim_General;
}
