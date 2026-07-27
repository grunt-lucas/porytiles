#ifndef GUARD_GLOBAL_FIELDMAP_H
#define GUARD_GLOBAL_FIELDMAP_H

// This include/global.fieldmap.h is intentionally malformed: the block comment below is never closed, so the lexer
// fails hard and the tolerant define scan returns no value. The masks below are real and readable by eye, which is
// the point: the scanner must record the file as unreadable rather than report a project that declares no masks.
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF /* unterminated block comment
#define METATILE_ATTR_LAYER_MASK    0xF000

#endif // GUARD_GLOBAL_FIELDMAP_H
