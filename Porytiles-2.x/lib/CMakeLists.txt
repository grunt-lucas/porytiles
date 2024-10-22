project(Porytiles2xLib CXX)

# Set up static library with all cpp files
FILE(GLOB CppSources */*.cpp)
add_library(Porytiles2xLib STATIC ${CppSources})
set(CANONICAL_LIB_NAME "porytiles")
set_target_properties(Porytiles2xLib PROPERTIES OUTPUT_NAME "${CANONICAL_LIB_NAME}")


# Set our public include directory as a PUBLIC dependency, we also mark private lib-specific headers as PRIVATE
target_include_directories(Porytiles2xLib PUBLIC ${PORYTILES_INCLUDE_DIR} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Export the library
# TODO 2.x : what's the difference between these two
export(TARGETS Porytiles2xLib FILE ${CMAKE_CURRENT_BINARY_DIR}/PorytilesLibraryTargets.cmake)
export(PACKAGE Porytiles2xLib)
