#include "panic/panic.hpp"

#include <cstdio>
#include <cstdlib>

namespace porytiles {

[[noreturn]] void panic_impl(const char *s) noexcept {
    std::fputs(s, stderr);
    std::abort();
}

} // namespace porytiles