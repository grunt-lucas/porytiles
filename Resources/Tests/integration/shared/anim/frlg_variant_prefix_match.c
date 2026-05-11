// Success case: InitTilesetAnim_General must be found even when InitTilesetAnim_General_Frlg exists.
// The parser must use exact name matching, not prefix matching.

static u16 sPrimaryTilesetAnimCounter;
static u16 sPrimaryTilesetAnimCounterMax;
static void (*sPrimaryTilesetAnimCallback)(u16);

static void TilesetAnim_General(u16);
static void TilesetAnim_General_Frlg(u16);
static void QueueAnimTiles_General_Flower(u16);
static void QueueAnimTiles_General_Frlg_Flower(u16);

const u16 gTilesetAnims_General_Flower_Frame0[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/0.4bpp");
const u16 gTilesetAnims_General_Flower_Frame1[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/1.4bpp");

const u16 *const gTilesetAnims_General_Flower[] = {
    gTilesetAnims_General_Flower_Frame0,
    gTilesetAnims_General_Flower_Frame1
};

const u16 gTilesetAnims_General_Frlg_Flower_Frame0[] = INCBIN_U16("data/tilesets/primary/general_frlg/anim/flower/0.4bpp");
const u16 gTilesetAnims_General_Frlg_Flower_Frame1[] = INCBIN_U16("data/tilesets/primary/general_frlg/anim/flower/1.4bpp");

const u16 *const gTilesetAnims_General_Frlg_Flower[] = {
    gTilesetAnims_General_Frlg_Flower_Frame0,
    gTilesetAnims_General_Frlg_Flower_Frame1
};

void InitTilesetAnim_General(void)
{
    sPrimaryTilesetAnimCounter = 0;
    sPrimaryTilesetAnimCounterMax = 256;
    sPrimaryTilesetAnimCallback = TilesetAnim_General;
}

void InitTilesetAnim_General_Frlg(void)
{
    sPrimaryTilesetAnimCounter = 0;
    sPrimaryTilesetAnimCounterMax = 640;
    sPrimaryTilesetAnimCallback = TilesetAnim_General_Frlg;
}

static void TilesetAnim_General(u16 timer)
{
    if (timer % 16 == 0)
        QueueAnimTiles_General_Flower(timer / 16);
}

static void TilesetAnim_General_Frlg(u16 timer)
{
    if (timer % 8 == 0)
        QueueAnimTiles_General_Frlg_Flower(timer / 8);
}

static void QueueAnimTiles_General_Flower(u16 timer)
{
    u16 i = timer % ARRAY_COUNT(gTilesetAnims_General_Flower);
    AppendTilesetAnimToBuffer(gTilesetAnims_General_Flower[i], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(508)), 4 * TILE_SIZE_4BPP);
}

static void QueueAnimTiles_General_Frlg_Flower(u16 timer)
{
    u16 i = timer % ARRAY_COUNT(gTilesetAnims_General_Frlg_Flower);
    AppendTilesetAnimToBuffer(gTilesetAnims_General_Frlg_Flower[i], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(508)), 4 * TILE_SIZE_4BPP);
}
