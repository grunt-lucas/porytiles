#include "porytiles2/infra/repos/ProjectPorytilesLayoutRepo.hpp"

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/aggregates/PorytilesLayout.hpp"
#include "porytiles2/templates/Panic.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

Result<void> ProjectPorytilesLayoutRepo::Save(const PorytilesLayout &layout) {
    // TODO : impl
    Panic("unimplemented");
}

Result<std::unique_ptr<PorytilesLayout>> ProjectPorytilesLayoutRepo::Load(const std::string &name) {
    // TODO : impl
    Panic("unimplemented");
}

} // namespace porytiles
