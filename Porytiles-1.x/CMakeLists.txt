project(Porytiles1x CXX)

# Enable compilation warnings for 1.x
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    # Cannot enable -Wextra in 1.x since some of our header-only dependencies throw warnings
    # that break the build.
    add_compile_options(-Wall -Wpedantic -Werror)
endif()

# Enable testing
enable_testing()

add_subdirectory(lib)
add_subdirectory(tests)
add_subdirectory(cli)
