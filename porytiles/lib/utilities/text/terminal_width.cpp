#include "porytiles/utilities/text/terminal_width.hpp"

#include <cstddef>
#include <cstdlib>
#include <optional>
#include <string>

#include <sys/ioctl.h>
#include <unistd.h>

namespace {

/// @brief Parses the COLUMNS environment variable into a positive width, if it is set and valid.
std::optional<std::size_t> width_from_columns_env()
{
    const char *columns = std::getenv("COLUMNS");
    if (columns == nullptr) {
        return std::nullopt;
    }
    try {
        std::size_t consumed = 0;
        const long value = std::stol(std::string{columns}, &consumed);
        // Require the whole value to parse and to be positive; ignore junk like "abc" or "80x".
        if (consumed == std::string{columns}.size() && value > 0) {
            return static_cast<std::size_t>(value);
        }
    }
    catch (...) {
        // Fall through to the next source on any parse failure.
    }
    return std::nullopt;
}

/// @brief Queries the terminal column count for @p fd via ioctl, if it is a terminal reporting a width.
std::optional<std::size_t> width_from_ioctl(const int fd)
{
    struct winsize ws{};
    if (ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return static_cast<std::size_t>(ws.ws_col);
    }
    return std::nullopt;
}

} // namespace

namespace porytiles {

std::size_t resolve_terminal_width(const int fd, const std::size_t fallback)
{
    if (const auto from_env = width_from_columns_env()) {
        return *from_env;
    }
    if (const auto from_ioctl = width_from_ioctl(fd)) {
        return *from_ioctl;
    }
    return fallback;
}

} // namespace porytiles
