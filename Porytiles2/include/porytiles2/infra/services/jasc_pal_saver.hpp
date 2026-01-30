#pragma once

#include <filesystem>

#include "gsl/pointers"

#include "porytiles2/infra/services/file_pal_saver.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief An implementation of FilePalSaver that saves palettes to JASC-PAL (Paintshop Pro) pal files.
 */
class JascPalSaver final : public FilePalSaver {
  public:
    explicit JascPalSaver(gsl::not_null<const TextFormatter *> format) : format_{format} {}

    [[nodiscard]] ChainableResult<void>
    save(const Palette<Rgba32, pal::max_size> &pal, const std::filesystem::path &path) const override;

  private:
    const TextFormatter *format_;
};

} // namespace porytiles2
