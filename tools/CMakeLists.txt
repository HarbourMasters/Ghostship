cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")

project(sm64-tmp VERSION 0.1.0 LANGUAGES C CXX)

if (MSVC)
find_path(DIRENT_INCLUDE_DIRS "dirent.h")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${DIRENT_INCLUDE_DIRS})
else()
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

if (MSVC)
set(INSTALL_DIR .)
endif()

set(mio0_SOURCES libmio0.c)
add_executable(mio0 ${mio0_SOURCES})
target_compile_definitions(mio0 PUBLIC MIO0_STANDALONE)

set(n64cksum_SOURCES n64cksum.c utils.c)
add_executable(n64cksum ${n64cksum_SOURCES})
target_compile_definitions(n64cksum PUBLIC N64CKSUM_STANDALONE)

set(textconv_SOURCES textconv.c utf8.c hashtable.c)
add_executable(textconv ${textconv_SOURCES})

set(patch_elf_32bit_SOURCES patch_elf_32bit.c)
add_executable(patch_elf_32bit ${patch_elf_32bit_SOURCES})

set(aifc_decode_SOURCES aifc_decode.c)
add_executable(aifc_decode ${aifc_decode_SOURCES})

set(aiff_extract_codebook_SOURCES aiff_extract_codebook.c)
add_executable(aiff_extract_cookbook ${aiff_extract_codebook_SOURCES})

add_dependencies(aiff_extract_cookbook tabledesign)

file(GLOB_RECURSE AUDIOFILE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} audiofile/*.cpp audiofile/*.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/audiofile)
add_library(audiofile STATIC ${AUDIOFILE_FILES})
set_target_properties(audiofile PROPERTIES CXX_STANDARD 11)

#tabledesign: $(LIBAUDIOFILE)
set(tabledesign_SOURCES  sdk-tools/tabledesign/codebook.c sdk-tools/tabledesign/estimate.c sdk-tools/tabledesign/print.c sdk-tools/tabledesign/getopt.c sdk-tools/tabledesign/tabledesign.c)
#tabledesign_CFLAGS  := -Iaudiofile -Wno-uninitialized
#tabledesign_LDFLAGS := -Laudiofile -laudiofile -lstdc++
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/audiofile)
if (MSVC)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/audiofile/include-windows)
endif()
add_executable(tabledesign ${tabledesign_SOURCES})
target_link_libraries(tabledesign PUBLIC audiofile)

if(MSVC)
find_path(GETOPT_INCLUDE_DIR getopt.h)
find_library(GETOPT_LIB getopt)
set(WINLIBS ${GETOPT_LIB} )
endif()

set(vadpcm_enc_SOURCES sdk-tools/adpcm/vadpcm_enc.c sdk-tools/adpcm/vpredictor.c sdk-tools/adpcm/quant.c sdk-tools/adpcm/util.c sdk-tools/adpcm/vencode.c)
#vadpcm_enc_CFLAGS  := -Wno-unused-result -Wno-uninitialized -Wno-sign-compare -Wno-absolute-value
add_executable(vadpcm_enc ${vadpcm_enc_SOURCES})
target_link_libraries(vadpcm_enc PRIVATE ${WINLIBS})
target_include_directories(vadpcm_enc PRIVATE ${GETOPT_INCLUDE_DIR})

set(extract_data_for_mio_SOURCES extract_data_for_mio.c)
add_executable(extract_data_for_mio ${extract_data_for_mio_SOURCES})

# tools required for extract_assets.py
add_custom_target(Tools
    DEPENDS
        aifc_decode
        mio0
        )
