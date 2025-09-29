#pragma once

#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

class AnsiStyledTextFormatter final : public TextFormatter {
  public:
    [[nodiscard]] std::string style(const std::string &text, Style styles) const override;
};

} // namespace porytiles2
