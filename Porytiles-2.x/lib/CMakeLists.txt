project(Porytiles2xLib CXX)

# Point this variable at our global, public include directory
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Porytiles-2.x/include)

# Set up static library with all cpp files
FILE(GLOB CppSources */*.cpp)
add_library(Porytiles2xLib STATIC ${CppSources})
set(CANONICAL_LIB_NAME "porytiles")
set_target_properties(Porytiles2xLib PROPERTIES OUTPUT_NAME "${CANONICAL_LIB_NAME}")


# Set our public include directory as a PUBLIC dependency, we also mark private lib-specific headers as PRIVATE
target_include_directories(Porytiles2xLib PUBLIC ${PROJECT_INCLUDE_DIR} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Export the library
export(TARGETS Porytiles2xLib FILE ${CMAKE_CURRENT_BINARY_DIR}/PorytilesLibraryTargets.cmake)
