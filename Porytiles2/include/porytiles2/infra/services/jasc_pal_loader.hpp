#pragma once

#include "porytiles2/infra/services/file_pal_loader.hpp"

namespace porytiles2 {

/**
 * @brief An implementation of FilePalLoader that loads palettes from JASC-PAL (Paintshop Pro) pal files.
 */
class JascPalLoader final : public FilePalLoader {
  public:
    JascPalLoader() = default;

    [[nodiscard]] Result<RgbaPal> load(const std::filesystem::path &path) const override;
};

} // namespace porytiles2
