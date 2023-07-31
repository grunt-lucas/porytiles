#ifndef PORYTILES_COMPILER_CONTEXT_H
#define PORYTILES_COMPILER_CONTEXT_H

#include <memory>

#include "config.h"
#include "types.h"

namespace porytiles {

enum CompilerMode { RAW, PRIMARY, SECONDARY };

struct CompilerContext {
  const Config &config;
  CompilerMode mode;
  std::unique_ptr<CompiledTileset> primaryTileset;

  explicit CompilerContext(const Config &config, CompilerMode mode) : config{config}, mode{mode}, primaryTileset{} {}
};

} // namespace porytiles

#endif // PORYTILES_COMPILER_CONTEXT_H