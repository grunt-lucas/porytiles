#ifndef PORYTILES_COMPILER_H
#define PORYTILES_COMPILER_H

#include <memory>

#include "ptcontext.h"
#include "types.h"

/**
 * TODO : fill in doc comment
 */

namespace porytiles {

/**
 * TODO : fill in doc comments
 */
std::unique_ptr<CompiledTileset> compile(PtContext &ctx, const DecompiledTileset &decompiledTileset);

} // namespace porytiles

#endif // PORYTILES_COMPILER_H
