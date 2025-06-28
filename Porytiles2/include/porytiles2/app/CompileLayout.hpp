#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/repos/PorymapLayoutRepo.hpp"
#include "porytiles2/domain/repos/PorytilesLayoutRepo.hpp"
#include "porytiles2/domain/services/LayoutCompilerService.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Use case for compiling a PorytilesLayout.
 */
class CompileLayout {
public:
  /**
   * @brief Constructs a CompileLayout use case with the given repositories and
   * compilation service.
   *
   * @param porytiles_repo A pointer to the PorytilesLayoutRepo for this use
   * case.
   * @param porymap_repo A pointer to the PorymapLayoutRepo for this use case.
   * @param compiler_service A pointer to the LayoutCompilerService for this use
   * case.
   */
  CompileLayout(std::unique_ptr<PorytilesLayoutRepo> porytiles_repo,
                std::unique_ptr<PorymapLayoutRepo> porymap_repo,
                std::unique_ptr<LayoutCompilerService> compiler_service)
      : porytiles_repo_{std::move(porytiles_repo)},
        porymap_repo_{std::move(porymap_repo)},
        compiler_service_{std::move(compiler_service)} {}

  /**
   * @brief Compiles the layout with the given name.
   *
   * @details
   * Given a layout by name, compile the PorytilesLayout assets into
   * PorymapLayout assets. Uses the use case's configured repos to load and save
   * the layout assets. Uses the given LayoutCompilerService to perform the
   * compilation operation.
   *
   * @param layout The name of the layout to compile.
   * @return An empty Result on success, otherwise an error description.
   */
  [[nodiscard]] Result<void> Compile(const std::string &layout) const;

private:
  std::unique_ptr<PorytilesLayoutRepo> porytiles_repo_;
  std::unique_ptr<PorymapLayoutRepo> porymap_repo_;
  std::unique_ptr<LayoutCompilerService> compiler_service_;
};

} // namespace porytiles
