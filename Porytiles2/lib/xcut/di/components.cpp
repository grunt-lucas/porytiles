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
    // TODO: make this mode configurable, macOS default Terminal.app only works with colors_256
    // We should probably have a detection chain:
    // - if $COLORTERM is set and is "truecolor", we can feel safe to enable "colors_24_bit"
    // - otherwise, check `tput colors`, if "256" then we can at least enable "colors_256"
    // - finally, if nothing then set "plain"
    return fruit::createComponent().bind<TextFormatter, AnsiStyledTextFormatter>().registerProvider(
        [] { return AnsiStyledTextFormatter{AnsiColorMode::colors_24_bit}; });
}

} // namespace porytiles2::di
