#include "decompiler.h"

#include <memory>

#include "ptcontext.h"
#include "types.h"

namespace porytiles {

std::unique_ptr<DecompiledTileset> decompile(PtContext &ctx, const CompiledTileset &compiledTileset)
{
  auto decompiled = std::make_unique<DecompiledTileset>();

  // TODO : fill in impl

  return decompiled;
}

} // namespace porytiles
