#include "porytiles2/xcut/di/components.hpp"

#include "fruit/fruit.h"

#include "porytiles2/utilities/text/ansi_styled_text_formatter.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

namespace porytiles2::di {

fruit::Component<TextFormatter> get_formatter_component(bool no_color)
{
    if (no_color) {
        return fruit::createComponent()
            .bind<TextFormatter, PlainTextFormatter>()
            .registerConstructor<PlainTextFormatter()>();
    }
    return fruit::createComponent().bind<TextFormatter, AnsiStyledTextFormatter>().registerProvider(
        [] { return AnsiStyledTextFormatter{AnsiColorMode::colors_24_bit}; });
}

} // namespace porytiles2::di
