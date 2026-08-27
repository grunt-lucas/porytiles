#pragma once

#include <filesystem>

#include "gsl/pointers"

#include "porytiles/infra/services/file_palette_saver.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

/// @brief An implementation of FilePaletteSaver that saves palettes to JASC-PAL (Paintshop Pro) palette files.
class JascPaletteSaver final : public FilePaletteSaver {
  public:
    explicit JascPaletteSaver(gsl::not_null<const TextFormatter *> format) : format_{format} {}

    [[nodiscard]] ChainableResult<void>
    save(const Palette<Rgba32, palette::max_size> &palette, const std::filesystem::path &path) const override;

  private:
    const TextFormatter *format_;
};

} // namespace porytiles
