#include "porytiles/utilities/panic/panic.hpp"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>

#include "cpptrace/cpptrace.hpp"

namespace {

bool stacktrace_enabled = true;

void print_stacktrace(std::size_t skip = 0)
{
    const auto trace = cpptrace::generate_trace(skip + 1);
    trace.print();
}

void print_panic_header(const std::filesystem::path &path, std::uint_least32_t line, std::string_view msg)
{
    std::cerr << "---------------------------------" << std::endl;
    std::cerr << "|             PANIC             |" << std::endl;
    std::cerr << "---------------------------------" << std::endl;
    std::cerr << std::format("{}:{}: {}", path.filename().string(), line, msg) << std::endl;
    std::cerr << std::endl;
}

} // namespace

namespace porytiles {

void set_panic_stacktrace_enabled(const bool enabled)
{
    stacktrace_enabled = enabled;
}

bool is_panic_stacktrace_enabled()
{
    return stacktrace_enabled;
}

void panic(const StringViewSourceLoc &s)
{
    const std::filesystem::path path{s.loc_.file_name()};
    print_panic_header(path, s.loc_.line(), s.msg_);
    if (stacktrace_enabled) {
        print_stacktrace(1);
    }
    std::abort();
}

void assert_or_panic(const bool condition, const StringViewSourceLoc &s)
{
    if (!condition) {
        const std::filesystem::path path{s.loc_.file_name()};
        print_panic_header(path, s.loc_.line(), s.msg_);
        if (stacktrace_enabled) {
            print_stacktrace(1);
        }
        std::abort();
    }
}

} // namespace porytiles
