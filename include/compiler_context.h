#ifndef PORYTILES_COMPILER_CONTEXT_H
#define PORYTILES_COMPILER_CONTEXT_H

#include "config.h"
#include "types.h"

namespace porytiles {

enum CompilerMode {
    RAW,
    PRIMARY,
    SECONDARY
};

struct CompilerContext {
    const Config& config;
    CompilerMode mode;
    const CompiledTileset* primaryTileset;

    explicit CompilerContext(const Config& config, CompilerMode mode) :
        config{config}, mode{mode}, primaryTileset{} {
    }
};

}

#endif // PORYTILES_COMPILER_CONTEXT_H