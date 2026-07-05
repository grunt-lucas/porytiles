const u16 gMetatiles_SecretBasePrimary[] = INCBIN_U16("data/tilesets/primary/secret_base/metatiles.bin");
const u16 gMetatileAttributes_SecretBasePrimary[] = INCBIN_U16("data/tilesets/primary/secret_base/metatile_attributes.bin");

#if !IS_FRLG
const u16 gMetatiles_General[] = INCBIN_U16("data/tilesets/primary/general/metatiles.bin");
const u16 gMetatileAttributes_General[] = INCBIN_U16("data/tilesets/primary/general/metatile_attributes.bin");
#else
const u16 gMetatiles_Building_Frlg[] = INCBIN_U16("data/tilesets/primary/building/metatiles.bin");
const u16 gMetatileAttributes_Building_Frlg[] = INCBIN_U16("data/tilesets/primary/building/metatile_attributes.bin");
#endif // IS_FRLG
