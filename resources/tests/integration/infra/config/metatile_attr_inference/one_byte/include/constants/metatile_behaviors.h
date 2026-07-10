#ifndef GUARD_METATILE_BEHAVIORS_H
#define GUARD_METATILE_BEHAVIORS_H

// Trimmed replica of the pokeemerald behaviors header: MB_ names as enum members plus an MB_INVALID define.
enum {
    MB_NORMAL,
    MB_SECRET_BASE_WALL,
    MB_TALL_GRASS,
    MB_LONG_GRASS,
};

#define MB_INVALID UCHAR_MAX

#endif // GUARD_METATILE_BEHAVIORS_H
