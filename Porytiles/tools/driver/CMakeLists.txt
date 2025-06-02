project(PorytilesDriver CXX)

include(FetchContent)
FetchContent_Declare(
        cli11_proj
        QUIET
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v2.5.0
)

FetchContent_MakeAvailable(cli11_proj)

FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS *.cpp)
add_executable(PorytilesDriver ${CppSources})
# Adding this to try to help CLion and CMake with inspections
target_compile_definitions(PorytilesDriver PRIVATE DOCTEST_CONFIG_DISABLE)
set_target_properties(PorytilesDriver PROPERTIES OUTPUT_NAME "porytiles2")
target_link_libraries(PorytilesDriver PRIVATE PorytilesLib)
target_link_libraries(PorytilesDriver PRIVATE CLI11::CLI11)
