cmake_minimum_required(VERSION 3.18)

project(Porytiles CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE ON)

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

add_subdirectory(Porytiles)
