#ifndef PORYTILES_COMPILER_H
#define PORYTILES_COMPILER_H

#include "config.h"
#include "types.h"

/**
 * TODO : fill in doc comment
 */

namespace porytiles {

/*
 * TODO : add a CompilerContext struct that contains a Config struct member as well as fields so that various compiler
 * functions can give good error messages with lots of context about what/where went wrong
 */

/**
 * TODO : fill in doc comments
 */
CompiledTileset compile(const Config& config, const DecompiledTileset& decompiledTileset);
CompiledTileset compileSecondary(const Config& config, const DecompiledTileset& decompiledTileset, const CompiledTileset& primaryTileset);

}

#endif // PORYTILES_COMPILER_H
