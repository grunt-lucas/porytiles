// Error: "Frame array '{}' for animation '{}' has no elements with a '_Frame' suffix."
// The referenced pointer array exists but its elements do not follow the _Frame naming convention.

static u16 sPrimaryTilesetAnimCounter;
static u16 sPrimaryTilesetAnimCounterMax;
static void (*sPrimaryTilesetAnimCallback)(u16);

static void TilesetAnim_General(u16);
static void QueueAnimTiles_General_Flower(u16);

const u16 gFlowerData0[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/0.4bpp");
const u16 gFlowerData1[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/1.4bpp");

const u16 *const gTilesetAnims_General_Flower[] = {
    gFlowerData0,
    gFlowerData1
};

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
