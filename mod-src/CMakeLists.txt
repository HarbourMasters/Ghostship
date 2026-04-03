cmake_minimum_required(VERSION 3.10)
project(ModTemplate)

# Find Python instead of tcc
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PARENT_DIR}
    ${PARENT_DIR}/include
    ${PARENT_DIR}/src
    ${PARENT_DIR}/libultraship/include
    ${PARENT_DIR}/libultraship/include/libultraship
)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*")

set(AMALGAMATED_FILE ${CMAKE_CURRENT_SOURCE_DIR}/output/mod.c)
get_filename_component(OUTPUT_DIR ${AMALGAMATED_FILE} DIRECTORY)

add_custom_command(
    OUTPUT ${AMALGAMATED_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    COMMAND Python3::Interpreter ${PARENT_DIR}/tools/merge.py 
            --out ${AMALGAMATED_FILE} 
            --includes ${INCLUDE_DIRS} 
            --srcs ${SRC_FILES}
    DEPENDS ${SRC_FILES} ${PARENT_DIR}/tools/merge.py
    VERBATIM
)

add_custom_target(generate_packaged_mod ALL DEPENDS ${AMALGAMATED_FILE})