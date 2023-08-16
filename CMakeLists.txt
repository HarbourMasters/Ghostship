cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

# Set the project version and language
project(sm64-tmp VERSION 0.1.0 LANGUAGES C CXX ASM)

if(APPLE)
  enable_language(OBJCXX)
endif()

# Set the minimum version of CMake and the deployment target for macOS
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")

# Set the C++ standard and enable the MSVC parallel build option
set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
set(CMAKE_C_STANDARD 11 CACHE STRING "The C standard to use")
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MT>)
#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

# Add a custom module path to locate additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
include(FindFontconfig)

if (WIN32)
include(cmake/automate-vcpkg.cmake)

if(MSVC)
set(VCPKG_TRIPLET x64-windows-static)
set(VCPKG_TARGET_TRIPLET x64-windows-static)
else()
set(VCPKG_TRIPLET x64-mingw-static)
set(VCPKG_TARGET_TRIPLET x64-mingw-static)
endif()

vcpkg_bootstrap()
if(MSVC)
vcpkg_install_packages(zlib bzip2 libpng getopt dirent libusb pthread)
else()
vcpkg_install_packages(fontconfig)
endif()
endif()

if (MSVC)
set(CPP "${CMAKE_C_COMPILER}" "/EP")
else()
set(CPP "${CMAKE_C_COMPILER}" "-E" "-P" "-Wno-trigraphs" "-x" "c")
endif()

# Set game compilation version
set(VERSION us)

# Add the tools subdirectory to the build
add_subdirectory(tools)

# Find Python 3 for the custom target
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Add a custom target to extract assets
add_custom_target(
    ExtractAssets
    COMMAND ${CMAKE_COMMAND} -E env "TOOLS_PATH=$<TARGET_FILE_DIR:mio0>" ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/extract_assets.py ${VERSION}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running asset extraction..."
    DEPENDS Tools
)

# Add a custom target to extract assets
add_custom_target(
    CleanAssets
    COMMAND  ${CMAKE_COMMAND} -E env "TOOLS_PATH=$<TARGET_FILE_DIR:mio0>" ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/extract_assets.py --clean
    COMMAND  ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_SOURCE_DIR}/.assets-local.txt
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running asset extraction..."
    DEPENDS Tools
)

# Add compile definitions for the target
add_compile_definitions(
    VERSION_US=1
    F3DEX_GBI_2=1
    _LANGUAGE_C
    _USE_MATH_DEFINES
    CIMGUI_DEFINE_ENUMS_AND_STRUCTS
)

# Find necessary libraries
if (UNIX AND NOT APPLE)
find_package(OpenGL REQUIRED)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${DIRECTORIES_TO_INCLUDE}
)

# Collect source files to build the executable
file(GLOB_RECURSE ALL_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "actors/*.h"
    "actors/*.c"
    "bin/*.c"
    "data/*.c"
    "levels/*.h"
    "levels/*.c"
    "lib/src/alBnkfNew.c"
    "lib/src/guLookAtRef.c"
    "lib/src/guMtxF2L.c"
    "lib/src/guNormalize.c"
    "lib/src/guOrthoF.c"
    "lib/src/guPerspectiveF.c"
    "lib/src/guRotateF.c"
    "lib/src/guScaleF.c"
    "lib/src/guTranslateF.c"
    "sound/*.c"
    "src/audio/*.h"
    "src/audio/*.c"
    "src/buffers/*.h"
    "src/buffers/*.c"
    "src/engine/*.h"
    "src/engine/*.c"
    "src/game/*.h"
    "src/game/*.c"
    "src/goddard/*.h"
    "src/goddard/*.c"
    "src/menu/*.h"
    "src/menu/*.c"
    "src/pc/*.h"
    "src/pc/*.c"
    "src/pc/*.cpp"
    "src/port/*.h"
    "src/port/*.c"
    "src/port/*.cpp"
    "text/*.c"
    "textures/*.h"
    "textures/*.c"
)

# Exclude specific files from the ALL_FILES list
list(FILTER ALL_FILES EXCLUDE REGEX ".*.inc.c")
list(FILTER ALL_FILES EXCLUDE REGEX "bin/eu/.*.c")
list(FILTER ALL_FILES EXCLUDE REGEX "src/game/main.c")
list(FILTER ALL_FILES EXCLUDE REGEX "src/pc/dlmalloc.c")

file(GLOB_RECURSE SKYBOXES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "textures/skyboxes/*.png")
foreach(skybox ${SKYBOXES})
  get_filename_component(OUTPUT_FILE_WLE ${skybox} NAME_WLE)
  get_filename_component(OUTPUT_DIR ${skybox} DIRECTORY)
  set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/bin/${OUTPUT_FILE_WLE}_skybox.c)
  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/bin
    VERBATIM COMMAND $<TARGET_FILE:skyconv> --type sky --split ${CMAKE_CURRENT_SOURCE_DIR}/${skybox} ${CMAKE_CURRENT_BINARY_DIR}/bin
    DEPENDS skyconv
  )
  list(APPEND ALL_FILES ${OUTPUT_FILE})
endforeach(skybox)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/level_headers.tmp
    COMMAND  ${CPP} -I${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/levels/level_headers.h.in > ${CMAKE_CURRENT_BINARY_DIR}/include/level_headers.tmp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
    OUTPUT include/level_headers.h
    COMMAND ${Python3_EXECUTABLE} tools/wrapinclude.py ${CMAKE_CURRENT_BINARY_DIR}/include/level_headers.tmp > ${CMAKE_CURRENT_BINARY_DIR}/include/level_headers.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/include/level_headers.tmp
)

list(APPEND ALL_FILES ${CMAKE_CURRENT_BINARY_DIR}/include/level_headers.h)


#==============================================================================#
# Generated Source Code Files                                                  #
#==============================================================================#

add_custom_command(
    OUTPUT  assets/mario_anim_data.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/mario_anims_converter.py > ${CMAKE_CURRENT_BINARY_DIR}/assets/mario_anim_data.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
list(APPEND ALL_FILES ${CMAKE_CURRENT_BINARY_DIR}/assets/mario_anim_data.c)

add_custom_command(
    OUTPUT  assets/demo_data.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/demo_data_converter.py ${CMAKE_CURRENT_SOURCE_DIR}/assets/demo_data.json -D VERSION_US > ${CMAKE_CURRENT_BINARY_DIR}/assets/demo_data.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
list(APPEND ALL_FILES ${CMAKE_CURRENT_BINARY_DIR}/assets/demo_data.c)

add_custom_command(
    OUTPUT  include/text_strings.h
    COMMAND $<TARGET_FILE:textconv> charmap.txt ${CMAKE_CURRENT_SOURCE_DIR}/include/text_strings.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/text_strings.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS textconv
)
list(APPEND ALL_FILES ${CMAKE_CURRENT_BINARY_DIR}/include/text_strings.h)

add_custom_command(
    OUTPUT  include/text_menu_strings.h
    COMMAND $<TARGET_FILE:textconv> charmap_menu.txt ${CMAKE_CURRENT_SOURCE_DIR}/include/text_menu_strings.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/text_menu_strings.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS textconv
)
list(APPEND ALL_FILES ${CMAKE_CURRENT_BINARY_DIR}/include/text_menu_strings.h)

add_executable(${PROJECT_NAME} ${ALL_FILES} ${GENERATED_SOURCES})

################################################################################
# MSVC runtime library
################################################################################
if (MSVC)
  if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	  get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
	  if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
		  string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
			  $<$<CONFIG:Debug>:
				  MultiThreadedDebug
			  >
			  $<$<CONFIG:Release>:
				  MultiThreaded
			  >
			  $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
		  )
	  elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
		  string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
			  $<$<CONFIG:Debug>:
				  MultiThreadedDebug
			  >
			  $<$<CONFIG:Release>:
				  MultiThreaded
			  >
			  $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
		  )
	  endif()
	  set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})
  endif()
endif()

#==============================================================================#
# Libultraship Integration                                                            #
#==============================================================================#

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/include
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/include/libultraship
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/log
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/debug
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/menu
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/utils
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/utils/binarytools
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/config
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/resource
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/resource/type
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/resource/factory
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/audio
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/window
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/window/gui
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/config
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/public
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/public/libultra
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/public/bridge
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/extern
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/extern/tinyxml2
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/libultraship/Lib/
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/libultraship/Lib/libjpeg/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/libultraship/Lib/spdlog/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/graphic/Fast3D/U64/PR
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/src/graphic
  ${SDL2_INCLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS}
)

add_subdirectory(libultraship ${CMAKE_CURRENT_SOURCE_DIR}/libultraship)

add_dependencies(${PROJECT_NAME} libultraship)
target_link_libraries(${PROJECT_NAME} PRIVATE libultraship)

if(APPLE)
  find_package(SDL2)
  target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)
endif()

#==============================================================================#
# Texture Generation                                                           #
#==============================================================================#

set (TEXTURE_ENCODING u8)
set(INC_C_FILES "")

file(GLOB_RECURSE ORGFILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "actors/*.png")
file(GLOB_RECURSE ORGFILES2 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "levels/*.png")
file(GLOB_RECURSE ORGFILES3 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "textures/*.png")
list (APPEND ORGFILES ${ORGFILES2})
list (APPEND ORGFILES ${ORGFILES3})
list(REMOVE_ITEM ORGFILES "levels/ending/cake.png")
list(FILTER ORGFILES EXCLUDE REGEX "textures/ipl3_raw/.*.png")
list(FILTER ORGFILES EXCLUDE REGEX "textures/skyboxes/.*.png")
foreach(orgfile ${ORGFILES})
  get_filename_component(OUTPUT_FILE_WLE ${orgfile} NAME_WLE)
  get_filename_component(OUTPUT_DIR ${orgfile} DIRECTORY)
  set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_DIR}/${OUTPUT_FILE_WLE}.inc.c)
  string(REPLACE "." ";" FORMAT ${OUTPUT_FILE_WLE})
  list(POP_BACK FORMAT FORMAT2)
  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    VERBATIM COMMAND $<TARGET_FILE:n64graphics> -s ${TEXTURE_ENCODING} -i ${OUTPUT_FILE} -g ${CMAKE_CURRENT_SOURCE_DIR}/${orgfile} -f ${FORMAT2}
    DEPENDS n64graphics
  )
  list(APPEND INC_C_FILES ${OUTPUT_FILE})
endforeach(orgfile)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/levels/ending/cake.inc.c
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/levels/ending
    COMMAND $<TARGET_FILE:skyconv> --type cake --split ${CMAKE_CURRENT_SOURCE_DIR}/levels/ending/cake.png ${CMAKE_CURRENT_BINARY_DIR}/levels/ending
    DEPENDS skyconv
  )
  list(APPEND INC_C_FILES ${CMAKE_CURRENT_BINARY_DIR}/levels/ending/cake.inc.c)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/text/us/define_text-tmp
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/text/us
    COMMAND ${CPP} -I${CMAKE_CURRENT_SOURCE_DIR}/text/us ${CMAKE_CURRENT_SOURCE_DIR}/text/define_text.inc.c > ${CMAKE_CURRENT_BINARY_DIR}/text/us/define_text-tmp
  )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/text/us/define_text.inc.c
    COMMAND $<TARGET_FILE:textconv> ${CMAKE_CURRENT_SOURCE_DIR}/charmap.txt  ${CMAKE_CURRENT_BINARY_DIR}/text/us/define_text-tmp ${CMAKE_CURRENT_BINARY_DIR}/text/us/define_text.inc.c
    VERBATIM
    DEPENDS textconv
    DEPENDS  ${CMAKE_CURRENT_BINARY_DIR}/text/us/define_text-tmp
  )

  list(APPEND INC_C_FILES ${CMAKE_CURRENT_BINARY_DIR}/text/us/define_text.inc.c)

#==============================================================================#
# Sound File Generation                                                        #
#==============================================================================#

execute_process(
    OUTPUT_VARIABLE endian_and_bitwidth
    COMMAND ${Python3_EXECUTABLE}  ${CMAKE_CURRENT_SOURCE_DIR}/tools/determine-endian-bitwidth.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
string(REPLACE " " ";" endian_and_bitwidth ${endian_and_bitwidth})

set (TABLE_FILES "")
file(GLOB_RECURSE AIFF_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "*.aiff")
foreach(aifffile ${AIFF_FILES})
  get_filename_component(OUTPUT_FILE_WLE ${aifffile} NAME_WLE)
  get_filename_component(OUTPUT_DIR ${aifffile} DIRECTORY)
  set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_DIR}/${OUTPUT_FILE_WLE}.table)
  list(APPEND TABLE_FILES ${OUTPUT_FILE})
  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    VERBATIM COMMAND ${CMAKE_COMMAND} -E env "TOOLS_PATH=$<TARGET_FILE_DIR:mio0>" $<TARGET_FILE:aiff_extract_cookbook> ${CMAKE_CURRENT_SOURCE_DIR}/${aifffile} > ${OUTPUT_FILE}
    DEPENDS aiff_extract_cookbook
    DEPENDS ${aifffile}
  )
endforeach(aifffile)

set(AIFC_FILES "")
foreach(aifffile ${AIFF_FILES})
  get_filename_component(OUTPUT_FILE_WLE ${aifffile} NAME_WLE)
  get_filename_component(OUTPUT_DIR ${aifffile} DIRECTORY)
  set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_DIR}/${OUTPUT_FILE_WLE}.aifc)
  list(APPEND AIFC_FILES ${OUTPUT_FILE})
  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    VERBATIM COMMAND $<TARGET_FILE:vadpcm_enc> -c ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_DIR}/${OUTPUT_FILE_WLE}.table ${CMAKE_CURRENT_SOURCE_DIR}/${aifffile}  ${OUTPUT_FILE}
    DEPENDS vadpcm_enc
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_DIR}/${OUTPUT_FILE_WLE}.table ${CMAKE_CURRENT_SOURCE_DIR}/${aifffile}
  )
endforeach(aifffile)

add_custom_command(
    OUTPUT sound/sound_data.ctl.inc.c sound/sound_data.tbl.inc.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/assemble_sound.py ${CMAKE_CURRENT_BINARY_DIR}/sound/samples/ ${CMAKE_CURRENT_SOURCE_DIR}/sound/sound_banks  ${CMAKE_CURRENT_BINARY_DIR}/sound/sound_data.ctl ${CMAKE_CURRENT_BINARY_DIR}/sound/ctl_header ${CMAKE_CURRENT_BINARY_DIR}/sound/sound_data.tbl ${CMAKE_CURRENT_BINARY_DIR}/sound/tbl_header -DVERSION_US ${endian_and_bitwidth}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/conv2hex.py ${CMAKE_CURRENT_BINARY_DIR}/sound/sound_data.ctl > ${CMAKE_CURRENT_BINARY_DIR}/sound/sound_data.ctl.inc.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/conv2hex.py ${CMAKE_CURRENT_BINARY_DIR}/sound/sound_data.tbl > ${CMAKE_CURRENT_BINARY_DIR}/sound/sound_data.tbl.inc.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${TABLE_FILES}
    DEPENDS ${AIFC_FILES}
)
list(APPEND INC_C_FILES ${CMAKE_CURRENT_BINARY_DIR}/sound/sound_data.ctl.inc.c ${CMAKE_CURRENT_BINARY_DIR}/sound/sound_data.tbl.inc.c)

file(GLOB_RECURSE SEQUENCES_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "sound/*.m64")

file(GLOB_RECURSE SEQUENCES_S_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "sound/*.s")
foreach(sequence_s ${SEQUENCES_S_FILES})
  get_filename_component(OUTPUT_FILE_WLE ${sequence_s} NAME_WLE)
  get_filename_component(OUTPUT_DIR ${sequence_s} DIRECTORY)
  set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_DIR}/${OUTPUT_FILE_WLE}.m64)

  if(APPLE)
    set(COPY_COMMAND i686-w64-mingw32-objcopy)
  else()
    set(COPY_COMMAND objcopy)
  endif()

  if(APPLE)
    set(AS_COMMAND i686-w64-mingw32-as)
  else()
    set(AS_COMMAND as)
  endif()

  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
    COMMAND  ${CPP} ${CMAKE_CURRENT_SOURCE_DIR}/${sequence_s} -I${CMAKE_CURRENT_SOURCE_DIR}/include -DVERSION_US | ${AS_COMMAND} -o ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_DIR}/${OUTPUT_FILE_WLE}.o
    VERBATIM COMMAND ${COPY_COMMAND} -j .rodata ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_DIR}/${OUTPUT_FILE_WLE}.o -O binary ${OUTPUT_FILE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${sequence_s}
  )
  list(APPEND SEQUENCES_FILES ${OUTPUT_FILE})
endforeach(sequence_s)

add_custom_command(
    OUTPUT sound/sequences.bin.inc.c sound/bank_sets.inc.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/assemble_sound.py --sequences ${CMAKE_CURRENT_BINARY_DIR}/sound/sequences.bin ${CMAKE_CURRENT_BINARY_DIR}/sound/sequences_header ${CMAKE_CURRENT_BINARY_DIR}/sound/bank_sets ${CMAKE_CURRENT_SOURCE_DIR}/sound/sound_banks ${CMAKE_CURRENT_SOURCE_DIR}/sound/sequences.json ${SEQUENCES_FILES} -DVERSION_US ${endian_and_bitwidth}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/conv2hex.py ${CMAKE_CURRENT_BINARY_DIR}/sound/sequences.bin > ${CMAKE_CURRENT_BINARY_DIR}/sound/sequences.bin.inc.c
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/conv2hex.py ${CMAKE_CURRENT_BINARY_DIR}/sound/bank_sets > ${CMAKE_CURRENT_BINARY_DIR}/sound/bank_sets.inc.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${SEQUENCES_FILES}
)
list(APPEND INC_C_FILES ${CMAKE_CURRENT_BINARY_DIR}/sound/sequences.bin.inc.c ${CMAKE_CURRENT_BINARY_DIR}/sound/bank_sets.inc.c)

add_custom_target(
  ConvertIncCFiles
  DEPENDS ${INC_C_FILES}
)

add_dependencies(${PROJECT_NAME} ConvertIncCFiles)
