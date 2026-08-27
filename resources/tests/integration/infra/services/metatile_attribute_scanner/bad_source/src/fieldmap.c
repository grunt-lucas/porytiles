#include "global.h"
#include "fieldmap.h"

// This src/fieldmap.c is intentionally malformed: the block comment below is never closed, so the lexer fails hard
// and parse_indexed_arrays() returns no value. The scanner must surface that as a warning rather than silently
// pretending the mask/shift tables are absent.
static const u32 sMetatileAttrMasks[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR] = 0x000001ff, /* unterminated block comment
};
