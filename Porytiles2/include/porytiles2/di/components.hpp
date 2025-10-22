#pragma once

#include "fruit/fruit.h"

#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2::di {

/**
 * @brief Component that provides TextFormatter based on runtime configuration.
 *
 * @details
 * This component conditionally binds either AnsiStyledTextFormatter or PlainTextFormatter based on the no_color
 * parameter. This demonstrates runtime conditional injection with Fruit DI.
 *
 * Usage:
 * ```C++
 * fruit::Injector<TextFormatter> injector(getFormatterComponent, false);
 * auto* formatter = injector.get<TextFormatter*>();
 * ```
 *
 * @param no_color If true, use PlainTextFormatter; otherwise use AnsiStyledTextFormatter
 * @return Component providing TextFormatter interface
 */
fruit::Component<TextFormatter> getFormatterComponent(bool no_color);

} // namespace porytiles2::di
