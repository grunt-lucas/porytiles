#ifndef PORYTILES_DECOMPILER_H
#define PORYTILES_DECOMPILER_H

#include <memory>

#include "./porytiles_context.h"
#include "./types.h"

namespace porytiles1 {

std::unique_ptr<DecompiledTileset> decompile(PorytilesContext &ctx, DecompilerMode mode,
                                             const CompiledTileset &compiledTileset,
                                             const std::unordered_map<std::size_t, Attributes> &attributesMap);

} // namespace porytiles1

#endif // PORYTILES_DECOMPILER_H
