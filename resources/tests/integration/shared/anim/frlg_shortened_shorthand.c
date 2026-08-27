// Success case: gTileset_General_Frlg whose animation arrays use the shortened shorthand "General".
// Mirrors stock pokeemerald-expansion, where TilesetAnim_General_Frlg queues sTilesetAnims_General_* arrays.

static u16 sPrimaryTilesetAnimCounter;
static u16 sPrimaryTilesetAnimCounterMax;
static void (*sPrimaryTilesetAnimCallback)(u16);

static void TilesetAnim_General_Frlg(u16);
static void QueueAnimTiles_General_Frlg_Flower(u16);
static void QueueAnimTiles_General_Water_Current_LandWatersEdge(u16);
static void QueueAnimTiles_General_SandWatersEdge(u16);

static const u16 sTilesetAnims_General_Flower_Frame0[] = INCBIN_U16("data/tilesets/primary/general_frlg/anim/flower/0.4bpp");
static const u16 sTilesetAnims_General_Flower_Frame1[] = INCBIN_U16("data/tilesets/primary/general_frlg/anim/flower/1.4bpp");

static const u16 *const sTilesetAnims_General_Flower[] = {
    sTilesetAnims_General_Flower_Frame0,
    sTilesetAnims_General_Flower_Frame1
};

static const u16 sTilesetAnims_General_Water_Current_LandWatersEdge_Frame0[] = INCBIN_U16("data/tilesets/primary/general_frlg/anim/water_current_landwatersedge/0.4bpp");
static const u16 sTilesetAnims_General_Water_Current_LandWatersEdge_Frame1[] = INCBIN_U16("data/tilesets/primary/general_frlg/anim/water_current_landwatersedge/1.4bpp");

static const u16 *const sTilesetAnims_General_Water_Current_LandWatersEdge[] = {
    sTilesetAnims_General_Water_Current_LandWatersEdge_Frame0,
    sTilesetAnims_General_Water_Current_LandWatersEdge_Frame1
};

static const u16 sTilesetAnims_General_SandWatersEdge_Frame0[] = INCBIN_U16("data/tilesets/primary/general_frlg/anim/sandwatersedge/0.4bpp");
static const u16 sTilesetAnims_General_SandWatersEdge_Frame1[] = INCBIN_U16("data/tilesets/primary/general_frlg/anim/sandwatersedge/1.4bpp");

static const u16 *const sTilesetAnims_General_SandWatersEdge[] = {
    sTilesetAnims_General_SandWatersEdge_Frame0,
    sTilesetAnims_General_SandWatersEdge_Frame1
};

void InitTilesetAnim_General_Frlg(void)
{
    sPrimaryTilesetAnimCounter = 0;
    sPrimaryTilesetAnimCounterMax = 640;
    sPrimaryTilesetAnimCallback = TilesetAnim_General_Frlg;
}

static void TilesetAnim_General_Frlg(u16 timer)
{
    if (timer % 8 == 0)
        QueueAnimTiles_General_SandWatersEdge(timer / 8);
    if (timer % 16 == 1)
        QueueAnimTiles_General_Water_Current_LandWatersEdge(timer / 16);
    if (timer % 16 == 2)
        QueueAnimTiles_General_Frlg_Flower(timer / 16);
}

static void QueueAnimTiles_General_Frlg_Flower(u16 timer)
{
    AppendTilesetAnimToBuffer(sTilesetAnims_General_Flower[timer % ARRAY_COUNT(sTilesetAnims_General_Flower)], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(508)), 4 * TILE_SIZE_4BPP);
}

static void QueueAnimTiles_General_Water_Current_LandWatersEdge(u16 timer)
{
    AppendTilesetAnimToBuffer(sTilesetAnims_General_Water_Current_LandWatersEdge[timer % ARRAY_COUNT(sTilesetAnims_General_Water_Current_LandWatersEdge)], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(416)), 48 * TILE_SIZE_4BPP);
}

static void QueueAnimTiles_General_SandWatersEdge(u16 timer)
{
    AppendTilesetAnimToBuffer(sTilesetAnims_General_SandWatersEdge[timer % ARRAY_COUNT(sTilesetAnims_General_SandWatersEdge)], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(464)), 18 * TILE_SIZE_4BPP);
}
