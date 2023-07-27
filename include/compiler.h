#ifndef PORYTILES_COMPILER_H
#define PORYTILES_COMPILER_H

#include "compiler_context.h"
#include "config.h"
#include "types.h"

/**
 * TODO : fill in doc comment
 */

namespace porytiles {

/**
 * TODO : fill in doc comments
 */
CompiledTileset compile(const CompilerContext& context, const DecompiledTileset& decompiledTileset);
CompiledTileset compilePrimary(const CompilerContext& context, const DecompiledTileset& decompiledTileset);
CompiledTileset compileSecondary(const CompilerContext& context, const DecompiledTileset& decompiledTileset, const CompiledTileset& primaryTileset);

}

#endif // PORYTILES_COMPILER_H
