#ifndef PORYTILES_COMPILER_H
#define PORYTILES_COMPILER_H

#include <memory>

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
std::unique_ptr<CompiledTileset> compile(const CompilerContext &context, const DecompiledTileset &decompiledTileset);

} // namespace porytiles

#endif // PORYTILES_COMPILER_H
