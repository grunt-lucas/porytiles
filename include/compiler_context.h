#ifndef PORYTILES_COMPILER_CONTEXT_H
#define PORYTILES_COMPILER_CONTEXT_H

#include "config.h"

namespace porytiles {

enum CompilerMode {
    RAW,
    PRIMARY,
    SECONDARY
};

struct CompilerContext {
    const Config& config;
    CompilerMode mode;

    explicit CompilerContext(const Config& config, CompilerMode mode) : config{config}, mode{mode} {
    }
};

}

#endif // PORYTILES_COMPILER_CONTEXT_H