project(PorytilesLibrary CXX)

find_package(PNG REQUIRED)

FILE(GLOB CppSources src/*.cpp)
add_library(PorytilesLibrary OBJECT ${CppSources})
set(CANONICAL_LIB_NAME "porytiles")
set_target_properties(PorytilesLibrary PROPERTIES OUTPUT_NAME "${CANONICAL_LIB_NAME}")
target_include_directories(PorytilesLibrary PUBLIC ${PROJECT_SOURCE_DIR}/../vendor/fmt-10.1.1/include)
target_include_directories(PorytilesLibrary PUBLIC ${PROJECT_SOURCE_DIR}/../vendor/png++-0.2.9)
target_include_directories(PorytilesLibrary PUBLIC ${PNG_INCLUDE_DIRS})
# Force dependents to include files like "porytiles/compiler.h", while internally we can simply use "compiler.h"
target_include_directories(PorytilesLibrary INTERFACE ${PROJECT_SOURCE_DIR}/include PRIVATE ${PROJECT_SOURCE_DIR}/include/${CANONICAL_LIB_NAME})
target_include_directories(PorytilesLibrary PRIVATE ${PROJECT_SOURCE_DIR}/../vendor/doctest-2.4.11)
target_include_directories(PorytilesLibrary PRIVATE ${PROJECT_SOURCE_DIR}/../vendor/fast-cpp-csv-parser)
target_link_libraries(PorytilesLibrary PRIVATE PNG::PNG)
