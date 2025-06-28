#include "porytiles2/app/CompileLayout.hpp"

#include <expected>
#include <memory>

#include "porytiles2/domain/services/LayoutCompilerService.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

Result<void> CompileLayout::Compile(const std::string &layout) const {
    Panic("unimplemented");
}

} // namespace porytiles
