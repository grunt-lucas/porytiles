#ifndef PORYTILES_DECOMPILER_H
#define PORYTILES_DECOMPILER_H

#include <memory>

#include "porytiles_context.h"
#include "types.h"

namespace porytiles {

std::unique_ptr<DecompiledTileset> decompile(PorytilesContext &ctx, const CompiledTileset &compiledTileset);

} // namespace porytiles

#endif // PORYTILES_DECOMPILER_H
