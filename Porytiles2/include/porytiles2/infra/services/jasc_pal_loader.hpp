#pragma once

#include <memory>

#include "gsl/pointers"

#include "porytiles2/infra/services/file_pal_loader.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/file_highlight_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief An implementation of FilePalLoader that loads palettes from JASC-PAL (Paintshop Pro) pal files.
 */
class JascPalLoader final : public FilePalLoader {
  public:
    explicit JascPalLoader(gsl::not_null<const TextFormatter *> format)
        : format_{format}, file_printer_{std::make_unique<FileHighlightPrinter>(format)}
    {
    }

    [[nodiscard]] ChainableResult<Palette<Rgba32, pal::max_size>>
    load(const std::filesystem::path &path) const override;

    [[nodiscard]] ChainableResult<Palette<Rgba32, pal::max_size>>
    load_with_wildcards(const std::filesystem::path &path) const override;

  private:
    const TextFormatter *format_;
    const std::unique_ptr<FileHighlightPrinter> file_printer_;
};

} // namespace porytiles2
