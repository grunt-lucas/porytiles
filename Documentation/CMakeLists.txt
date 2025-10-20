include(FetchContent)
FetchContent_Declare(
        doxygen-awesome-css
        GIT_REPOSITORY "https://github.com/jothepro/doxygen-awesome-css"
        GIT_TAG "v2.4.0")
FetchContent_MakeAvailable(doxygen-awesome-css)
FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)

find_package(Doxygen REQUIRED)

# Find all the public headers
get_target_property(PORYTILES2_PUBLIC_HEADER_DIR Porytiles2Lib INTERFACE_INCLUDE_DIRECTORIES)
file(GLOB_RECURSE PORYTILES2_PUBLIC_HEADERS ${PORYTILES2_PUBLIC_HEADER_DIR}/*.hpp)

# This will be the main output of our command
set(DOXYGEN_PROJECT_PREFIX "${PROJECT_SOURCE_DIR}")
set(DOXYGEN_INPUT_DIRS "${PROJECT_SOURCE_DIR}/README.md ${PROJECT_SOURCE_DIR}/Porytiles2/include ${PROJECT_SOURCE_DIR}/Porytiles2/lib ${PROJECT_SOURCE_DIR}/Porytiles2/tools")
set(DOXYGEN_MAIN_README "${PROJECT_SOURCE_DIR}/README.md")
set(DOXYGEN_HTML_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/header.html")
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_HTML_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# Replace variables inside @@ with the current values
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

# Set up run for Doxygen, add as target
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
add_custom_command(OUTPUT ${DOXYGEN_HTML_INDEX_FILE}
        DEPENDS ${PORYTILES2_PUBLIC_HEADERS}
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
        MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
        COMMENT "Generating Doxygen docs...")
add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_HTML_INDEX_FILE})
