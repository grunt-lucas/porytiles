project(PorytilesLibrary CXX)

# Point this variable at our global, public include directory
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# Set up static library with all cpp files
FILE(GLOB CppSources */*.cpp)
add_library(PorytilesLibrary STATIC ${CppSources})
set(CANONICAL_LIB_NAME "porytiles")
set_target_properties(PorytilesLibrary PROPERTIES OUTPUT_NAME "${CANONICAL_LIB_NAME}")


# Set our public include directory as a PUBLIC dependency, we also mark private lib-specific headers as PRIVATE
target_include_directories(PorytilesLibrary PUBLIC ${PROJECT_INCLUDE_DIR} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Export the library
export(TARGETS PorytilesLibrary FILE ${CMAKE_CURRENT_BINARY_DIR}/PorytilesLibraryTargets.cmake)
