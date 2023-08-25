#ifndef PORYTILES_DECOMPILER_H
#define PORYTILES_DECOMPILER_H

#include <memory>

#include "ptcontext.h"
#include "types.h"

namespace porytiles {

std::unique_ptr<DecompiledTileset> decompile(PtContext &ctx, const CompiledTileset &compiledTileset);

} // namespace porytiles

#endif // PORYTILES_DECOMPILER_H
