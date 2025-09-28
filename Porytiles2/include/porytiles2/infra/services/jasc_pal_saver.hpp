#pragma once

#include "porytiles2/infra/services/file_pal_saver.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief An implementation of FilePalSaver that saves palettes to JASC-PAL (Paintshop Pro) pal files.
 */
class JascPalSaver final : public FilePalSaver {
  public:
    JascPalSaver() = default;

    [[nodiscard]] ChainableResult<void> save(const RgbaPal &pal, const std::filesystem::path &path) const override;
};

} // namespace porytiles2
