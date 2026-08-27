#ifndef GUARD_GLOBAL_FIELDMAP_H
#define GUARD_GLOBAL_FIELDMAP_H

// A mask define with conflicting values inside a conditional the tolerant scan cannot decide. The scan records the
// name as ambiguous and emits a recoverable warning rather than aborting.
#ifdef SOME_UNKNOWABLE_BUILD_FLAG
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF
#else
#define METATILE_ATTR_BEHAVIOR_MASK 0x01FF
#endif

#endif // GUARD_GLOBAL_FIELDMAP_H
