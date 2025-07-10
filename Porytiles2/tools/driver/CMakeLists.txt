project(Porytiles2Driver CXX)

include(FetchContent)
FetchContent_Declare(
        cli11_proj
        QUIET
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v2.5.0
)

FetchContent_MakeAvailable(cli11_proj)

FILE(GLOB_RECURSE CppSources CONFIGURE_DEPENDS *.cpp)
add_executable(Porytiles2Driver ${CppSources})
# Adding this to try to help CLion and CMake with inspections
target_compile_definitions(Porytiles2Driver PRIVATE DOCTEST_CONFIG_DISABLE)
set_target_properties(Porytiles2Driver PROPERTIES OUTPUT_NAME "porytiles2")
target_include_directories(Porytiles2Driver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Porytiles2Driver PRIVATE Porytiles2Lib)
target_link_libraries(Porytiles2Driver PRIVATE CLI11::CLI11)
