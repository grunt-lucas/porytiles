#include "porytiles2/infra/repos/ProjectPorymapLayoutRepo.hpp"

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/aggregates/PorymapLayout.hpp"
#include "porytiles2/templates/Panic.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

Result<void> ProjectPorymapLayoutRepo::Save(const PorymapLayout &layout) {
  // TODO : impl
  Panic("unimplemented");
}

Result<std::unique_ptr<PorymapLayout>>
ProjectPorymapLayoutRepo::Load(const std::string &name) {
  // TODO : impl
  Panic("unimplemented");
}

} // namespace porytiles
