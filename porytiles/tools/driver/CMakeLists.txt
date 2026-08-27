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
# Headers listed as sources so IDEs treat them as project files (compile-neutral;
# CMake ignores .hpp). See porytiles/lib/CMakeLists.txt for the full rationale.
FILE(GLOB_RECURSE DriverHeaders CONFIGURE_DEPENDS *.hpp)
add_executable(PorytilesDriver ${CppSources} ${DriverHeaders})
# Adding this to try to help CLion and CMake with inspections
target_compile_definitions(PorytilesDriver PRIVATE DOCTEST_CONFIG_DISABLE)
# Driver TU consumes PORYTILES_BUILD_VERSION directly (in main.cpp); PorytilesLib's PRIVATE def doesn't reach this target.
target_compile_definitions(PorytilesDriver PRIVATE PORYTILES_BUILD_VERSION_=${PORYTILES_BUILD_VERSION_})
target_compile_definitions(PorytilesDriver PRIVATE PORYTILES_BUILD_DATE_=${PORYTILES_BUILD_DATE_})
set_target_properties(PorytilesDriver PROPERTIES OUTPUT_NAME "porytiles")
target_include_directories(PorytilesDriver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PorytilesDriver PRIVATE PorytilesLib)
target_link_libraries(PorytilesDriver PRIVATE CLI11::CLI11)
target_link_libraries(PorytilesDriver PRIVATE fruit)

install(TARGETS PorytilesDriver
        RUNTIME DESTINATION bin)
