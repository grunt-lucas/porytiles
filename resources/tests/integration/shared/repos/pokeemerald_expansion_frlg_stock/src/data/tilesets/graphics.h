const u32 gTilesetTiles_General[] = INCBIN_U32("data/tilesets/primary/general/tiles.4bpp.lz");

const u16 gTilesetPalettes_General[][16] =
{
    INCBIN_U16("data/tilesets/primary/general/palettes/00.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/01.gbapal"),
};

#if IS_FRLG
const u32 gTilesetTiles_Building_Frlg[] = INCBIN_U32("data/tilesets/primary/building/tiles.4bpp.lz");

const u16 gTilesetPalettes_Building_Frlg[][16] =
{
    INCBIN_U16("data/tilesets/primary/building/palettes/00.gbapal"),
    INCBIN_U16("data/tilesets/primary/building/palettes/01.gbapal"),
};
#endif // IS_FRLG
