// NOTE: gTilesetTiles_Broken is intentionally absent. The header references it, but no INCBIN/INCGFX
// declaration exists for it in any scanned file, so artifact path resolution must fail with a diagnostic.

const u16 gTilesetPalettes_Broken[][16] =
{
    INCBIN_U16("data/tilesets/primary/broken/palettes/00.gbapal"),
};
