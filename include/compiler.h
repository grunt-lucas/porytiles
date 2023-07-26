#ifndef PORYTILES_COMPILER_H
#define PORYTILES_COMPILER_H

#include "config.h"
#include "types.h"

/**
 * TODO : fill in doc comment
 */

namespace porytiles {

/**
 * TODO : fill in doc comments
 */
CompiledTileset compilePrimary(const Config& config, const DecompiledTileset& decompiledTileset);
CompiledTileset compileSecondary(const Config& config, const DecompiledTileset& decompiledTileset, const CompiledTileset& primaryTileset);

}

#endif // PORYTILES_COMPILER_H
