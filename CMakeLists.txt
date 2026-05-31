cmake_minimum_required(VERSION 3.20)

project(Porytiles CXX)

# Debug builds: no optimization, full debug info
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fstandalone-debug -fno-limit-debug-info -fno-omit-frame-pointer"
        CACHE STRING "Flags used by the CXX compiler during DEBUG builds." FORCE)

# Release builds: optimized with debug symbols for useful stacktraces
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Zi"
            CACHE STRING "Flags used by the CXX compiler during RELEASE builds." FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG /OPT:REF /OPT:ICF"
            CACHE STRING "Flags used by the linker during RELEASE builds." FORCE)
else ()
    # -gfull embeds complete debug info in the binary (no external .o/.dSYM needed)
    # -gline-tables-only would be smaller but only provides file:line, no variable info
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g -fno-omit-frame-pointer"
            CACHE STRING "Flags used by the CXX compiler during RELEASE builds." FORCE)
endif ()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(FETCHCONTENT_QUIET OFF)

# Use libc++ when compiling with Clang. On macOS this is already the default,
# but on Linux (e.g. WSL) Clang defaults to libstdc++ which may lack C++23 headers.
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)

    # fmt's format.h (and potentially other deps) calls malloc/free without
    # including <cstdlib>, relying on transitive includes. Newer libc++
    # (LLVM 18+) has pruned those transitive paths, so any TU instantiating
    # fmt's allocator fails to compile. Apple Clang ships an older libc++
    # snapshot where the transitive includes still exist, which is why this
    # only bites on Linux/LLVM. Force-include cstdlib globally for C++ TUs
    # only (cstdlib is a C++ header; C TUs would fail to find it).
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-include> $<$<COMPILE_LANGUAGE:CXX>:cstdlib>)
endif ()

# Enable compilation warnings
if (MSVC)
    add_compile_options(/W4 /WX)
else ()
    # Cannot enable -Wextra since some of our header-only dependencies throw
    # warnings that break the build, and I'd rather leave -Werror enabled.
    #
    # -Werror must be disabled for now, due to a dependency issue:
    #
    # Porytiles/vendor/fast-cpp-csv-parser/csv.h: In constructor
    # ‘io::CSVReader<column_count, trim_policy, quote_policy, overflow_policy, comment_policy>::CSVReader(Args&& ...)
    # [with Args = {const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&};
    # unsigned int column_count = 4; trim_policy = io::trim_chars<' ', '\011'>; quote_policy = io::no_quote_escape<','>;
    # overflow_policy = io::throw_on_overflow; comment_policy = io::no_comment]’:
    # Porytiles/vendor/fast-cpp-csv-parser/csv.h:410:14: error:
    # ‘char* __builtin_strncpy(char*, const char*, long unsigned int)’ specified bound 1025 equals destination size [-Werror=stringop-truncation]
    #  410 |       strncpy(this->file_name, file_name, sizeof(this->file_name));
    #      |              ^
    #
    # add_compile_options(-Wall -Wpedantic -Werror)
    add_compile_options(-Wall -Wpedantic)
endif ()

add_subdirectory(Legacy)
add_subdirectory(Porytiles)
add_subdirectory(Documentation)
