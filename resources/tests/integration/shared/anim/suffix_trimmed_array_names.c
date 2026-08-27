// Success case: frame pointer arrays whose names carry a _Frames or _VDests suffix after the animation name.
// The suffix is trimmed when resolving the animation name; the array is still matched by its full identifier.

static u16 sPrimaryTilesetAnimCounter;
static u16 sPrimaryTilesetAnimCounterMax;
static void (*sPrimaryTilesetAnimCallback)(u16);

static void TilesetAnim_General(u16);
static void QueueAnimTiles_General_Flower(u16);
static void QueueAnimTiles_General_Door(u16);

const u16 gTilesetAnims_General_Flower_Frame0[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/0.4bpp");
const u16 gTilesetAnims_General_Flower_Frame1[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/1.4bpp");

const u16 *const gTilesetAnims_General_Flower_Frames[] = {
    gTilesetAnims_General_Flower_Frame0,
    gTilesetAnims_General_Flower_Frame1
};

const u16 gTilesetAnims_General_Door_Frame0[] = INCBIN_U16("data/tilesets/primary/general/anim/door/0.4bpp");
const u16 gTilesetAnims_General_Door_Frame1[] = INCBIN_U16("data/tilesets/primary/general/anim/door/1.4bpp");

const u16 *const gTilesetAnims_General_Door_VDests[] = {
    gTilesetAnims_General_Door_Frame0,
    gTilesetAnims_General_Door_Frame1
};

void InitTilesetAnim_General(void)
{
    sPrimaryTilesetAnimCounter = 0;
    sPrimaryTilesetAnimCounterMax = 256;
    sPrimaryTilesetAnimCallback = TilesetAnim_General;
}

static void TilesetAnim_General(u16 timer)
{
    if (timer % 16 == 0)
        QueueAnimTiles_General_Flower(timer / 16);
    if (timer % 16 == 1)
        QueueAnimTiles_General_Door(timer / 16);
}

static void QueueAnimTiles_General_Flower(u16 timer)
{
    AppendTilesetAnimToBuffer(gTilesetAnims_General_Flower_Frames[timer % ARRAY_COUNT(gTilesetAnims_General_Flower_Frames)], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(508)), 4 * TILE_SIZE_4BPP);
}

static void QueueAnimTiles_General_Door(u16 timer)
{
    AppendTilesetAnimToBuffer(gTilesetAnims_General_Door_VDests[timer % ARRAY_COUNT(gTilesetAnims_General_Door_VDests)], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP(640)), 4 * TILE_SIZE_4BPP);
}
