#include "panic/panic.hpp"

#include <cstdio>
#include <cstdlib>

namespace porytiles11 {

[[noreturn]] void PanicImpl(const char *s) noexcept {
    std::fputs(s, stderr);
    std::abort();
}

} // namespace porytiles11