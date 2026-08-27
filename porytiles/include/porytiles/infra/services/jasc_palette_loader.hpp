#pragma once

#include <filesystem>
#include <memory>

#include "gsl/pointers"

#include "porytiles/infra/services/file_palette_loader.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/file_highlight_printer.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

/// @brief An implementation of FilePaletteLoader that loads palettes from JASC-PAL (Paintshop Pro) palette files.
class JascPaletteLoader final : public FilePaletteLoader {
  public:
    explicit JascPaletteLoader(gsl::not_null<const TextFormatter *> format)
        : format_{format}, file_printer_{std::make_unique<FileHighlightPrinter>(format)}
    {
    }

    [[nodiscard]] ChainableResult<Palette<Rgba32, palette::max_size>>
    load(const std::filesystem::path &path) const override;

    [[nodiscard]] ChainableResult<Palette<Rgba32, palette::max_size>>
    load_with_wildcards(const std::filesystem::path &path) const override;

  private:
    const TextFormatter *format_;
    const std::unique_ptr<FileHighlightPrinter> file_printer_;
};

} // namespace porytiles
