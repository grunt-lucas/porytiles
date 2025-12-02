#pragma once

#include "porytiles2/infra/services/file_pal_loader.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief An implementation of FilePalLoader that loads palettes from JASC-PAL (Paintshop Pro) pal files.
 */
class JascPalLoader final : public FilePalLoader {
  public:
    JascPalLoader() = default;

    [[nodiscard]] ChainableResult<Palette<Rgba32, pal::max_size>>
    load(const std::filesystem::path &path) const override;
};

} // namespace porytiles2
