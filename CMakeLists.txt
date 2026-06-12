cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# Set the project version and language
project(Ghostship VERSION 2.1.0 LANGUAGES C CXX ASM)
include(FetchContent)

include(cmake/ghostship-cvars.cmake)
include(cmake/lus-cvars.cmake)

set(NATO_PHONETIC_ALPHABET
  "Alfa" "Bravo" "Charlie" "Delta" "Echo" "Foxtrot" "Golf" "Hotel"
  "India" "Juliett" "Kilo" "Lima" "Mike" "November" "Oscar" "Papa"
  "Quebec" "Romeo" "Sierra" "Tango" "Uniform" "Victor" "Whiskey"
  "Xray" "Yankee" "Zulu"
)

# Get the patch version number from the project version
math(EXPR PATCH_INDEX "${PROJECT_VERSION_PATCH}")

# Use the patch number to select the correct word
list(GET NATO_PHONETIC_ALPHABET ${PATCH_INDEX} PROJECT_PATCH_WORD)

set(PROJECT_BUILD_NAME "Mary Celeste ${PROJECT_PATCH_WORD}" CACHE STRING "" FORCE)
set(PROJECT_TEAM "Lywx" CACHE STRING "" FORCE)

# Get Git Information
execute_process(
    COMMAND git branch --show-current
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_PROJECT_GIT_BRANCH "${GIT_BRANCH}" CACHE STRING "Git branch" FORCE)

execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get only the first 7 characters of the hash
string(SUBSTRING "${GIT_COMMIT_HASH}" 0 7 SHORT_COMMIT_HASH)

set(CMAKE_PROJECT_GIT_COMMIT_HASH "${SHORT_COMMIT_HASH}" CACHE STRING "Git commit hash" FORCE)

execute_process(
    COMMAND git describe --tags --abbrev=0 --exact-match HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT GIT_COMMIT_TAG)
    set(GIT_COMMIT_TAG "" CACHE STRING "Git commit tag" FORCE)
endif()

set(CMAKE_PROJECT_GIT_COMMIT_TAG "${GIT_COMMIT_TAG}" CACHE STRING "Git commit tag" FORCE)

if(APPLE)
  enable_language(OBJCXX)
endif()

# Set the minimum version of CMake and the deployment target for macOS
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")

# Set the C++ standard and enable the MSVC parallel build option
set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
set(CMAKE_C_STANDARD 11 CACHE STRING "The C standard to use")
# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

# Add a custom module path to locate additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(YAML_CPP_STATIC_DEFINE ON)
add_compile_definitions(YAML_CPP_STATIC_DEFINE)

if (WIN32)
include(cmake/automate-vcpkg.cmake)
set(VCPKG_TRIPLET x64-windows-static)
set(VCPKG_TARGET_TRIPLET x64-windows-static)
vcpkg_bootstrap()
vcpkg_install_packages(zlib bzip2 libzip libpng sdl2 glew glfw3 nlohmann-json tinyxml2 spdlog mbedtls)

add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

if (MSVC)
set(CPP "${CMAKE_C_COMPILER}" "/EP")
else()
set(CPP "${CMAKE_C_COMPILER}" "-E" "-P" "-Wno-trigraphs" "-x" "c")
endif()

if(IOS)
    set(PLATFORM "OS64")
    include(cmake/ios.toolchain.cmake)
    add_compile_definitions(
        PLATFORM_IOS=1
    )
    set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "YOUR_TEAM_ID")
    set(PROJECT_ID "${BUNDLE_ID}")
endif()

################################################################################
# Sources
################################################################################
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/src/port/build.c.in ${CMAKE_CURRENT_SOURCE_DIR}/src/port/build.c @ONLY)

################################################################################
# Set target arch type if empty. Visual studio solution generator provides it.
################################################################################
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(NOT CMAKE_VS_PLATFORM_NAME)
        set(CMAKE_VS_PLATFORM_NAME "x64")
    endif()
    message("${CMAKE_VS_PLATFORM_NAME} architecture in use")

    if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64"
        OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32"))
        message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
    endif()
endif()

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Global configuration types
################################################################################
if (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
set(CMAKE_C_FLAGS_DEBUG "-g -ffast-math -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -ffast-math -DDEBUG")
# Optimization level -O2 and higher cause graphical 3D glitches due to undefined behavior in some of the C code (on both PC and Switch).
# The optimization that causes this issue is -fstrict-aliasing. The camera is locked in place, often out-of-bounds, and renders the game unplayable.
set(CMAKE_C_FLAGS_RELEASE "-O3 -fno-strict-aliasing -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG")
add_compile_definitions(
    _LARGEFILE64_SOURCE=1
    _POSIX_C_SOURCE=200809L
)
else()
set(CMAKE_C_FLAGS_RELEASE "-O1 -fno-strict-aliasing -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O1 -DNDEBUG")
set(CMAKE_OBJCXX_FLAGS_RELEASE "-O1 -DNDEBUG")
endif()

# Set game compilation version
set(VERSION us)

option(USE_SATELLA "Connect to Satella at startup for mod signing and online relay features" ON)

if(NOT USE_SATELLA)
    message(WARNING
        "\n"
        "  USE_SATELLA is OFF — Satella relay connection disabled.\n"
        "  The following features will NOT work:\n"
        "    * Mod signature verification (public keys are fetched from Satella)\n"
        "    * Online / relay features in mods (Relay API becomes a no-op)\n"
        "  Mods that depend on networking will fail silently at runtime.\n"
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch" OR CMAKE_SYSTEM_NAME STREQUAL "CafeOS" OR NOT USE_SATELLA)
    set(USE_NETWORKING_DEFAULT OFF)
else()
    set(USE_NETWORKING_DEFAULT ON)
endif()
option(USE_NETWORKING "Enable the Satella online relay / networking features" ${USE_NETWORKING_DEFAULT})

if(NOT USE_SATELLA AND USE_NETWORKING)
    set(USE_NETWORKING OFF CACHE BOOL "Disabled because USE_SATELLA=OFF" FORCE)
endif()

set(SKIP_XCODE_VERSION_CHECK ON)

# Add compile definitions for the target
add_compile_definitions(
    VERSION_US=1
    ENABLE_RUMBLE=1
    F3D_OLD=1
    F3D_GBI=1
    GBI_FLOATS=1
    _LANGUAGE_C
    _USE_MATH_DEFINES
    AVOID_UB=1
    ENABLE_OPENGL=1
)

# Find necessary libraries
if (UNIX AND NOT APPLE)
find_package(OpenGL REQUIRED)
endif()

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/properties.h.in ${CMAKE_CURRENT_SOURCE_DIR}/properties.h @ONLY)

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
    "lib/src/guLookAtF.c"
    "lib/src/guTranslateF.c"
    "lib/src/osAiSetFrequency.c"
    "include/**/*"
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
    "src/port/Enhancements/*.h"
    "src/port/Enhancements/*.cpp"
    "src/port/Enhancements/game-interactor/*.h"
    "src/port/Enhancements/game-interactor/*.cpp"
    "src/port/importer/*.cpp"
    "src/port/importer/types/*.cpp"
    "src/port/mods/*.h"
    "src/port/mods/*.cpp"
    "src/port/mods/mirror/*.h"
    "src/port/mods/mirror/*.cpp"
    "src/port/net/*.h"
    "src/port/net/*.cpp"
    "text/*.c"
    "textures/*.h"
    "textures/*.c"
    "Resource.rc"
)

# Exclude specific files from the ALL_FILES list
list(FILTER ALL_FILES EXCLUDE REGEX ".*.inc.c")
list(FILTER ALL_FILES EXCLUDE REGEX "bin/eu/.*.c")
list(FILTER ALL_FILES EXCLUDE REGEX "src/game/main.c")
list(FILTER ALL_FILES EXCLUDE REGEX "src/pc/dlmalloc.c")

# Create source groups that match the real file directory paths
foreach(source IN LISTS ALL_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_adjusted "${source_path}")
    source_group("${source_path_adjusted}" FILES "${source}")
endforeach()

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ios)

    set(STORYBOARD_FILE ${IOS_DIR}/Launch.storyboard)
    set(IMAGE_FILES ${IOS_DIR}/PoweredBy.png)
    set(ICON_FILES ${IOS_DIR}/Icon.png)

    list(APPEND ALL_FILES ${STORYBOARD_FILE} ${IMAGE_FILES} ${ICON_FILES})

    add_executable(${PROJECT_NAME} ${ALL_FILES})
    set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${IOS_DIR}/plist.in
        RESOURCE "${IMAGE_FILES};${STORYBOARD_FILE};${ICON_FILES}"
    )
else()
    add_executable(${PROJECT_NAME} ${ALL_FILES})
endif()

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

FetchContent_Declare(
    dr_libs
    GIT_REPOSITORY https://github.com/mackron/dr_libs.git
    GIT_TAG da35f9d6c7374a95353fd1df1d394d44ab66cf01
)
FetchContent_MakeAvailable(dr_libs)

#==============================================================================#
# Libultraship Integration                                                     #
#==============================================================================#

# Removes MPQ/OTR support
set(EXCLUDE_MPQ_SUPPORT TRUE CACHE BOOL "")
set(ENABLE_EXP_AUTO_CONFIGURE_CONTROLLERS ON CACHE BOOL "")
add_compile_definitions(EXCLUDE_MPQ_SUPPORT)

if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    find_package(SDL2)
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/include
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/include/libultraship
  ${SDL2_INCLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS}
  ${dr_libs_SOURCE_DIR}
)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(ENABLE_SCRIPTING ON CACHE BOOL "" FORCE)
else()
    #=================== tinyxml2 ===================
    # devkitPro portlibs may have tinyxml2 headers/lib but no CMake config that
    # exports tinyxml2::tinyxml2. Always fetch from source so both this project
    # and libultraship's find_package(tinyxml2 REQUIRED) get a proper target.
    set(tinyxml2_BUILD_TESTING OFF)
    FetchContent_Declare(
        tinyxml2
        GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
        GIT_TAG 10.0.0
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(tinyxml2)
endif()
add_subdirectory(libultraship ${CMAKE_CURRENT_SOURCE_DIR}/libultraship)
add_dependencies(${PROJECT_NAME} libultraship)
if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    target_link_libraries(${PROJECT_NAME} PRIVATE libultraship)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,libultraship>")
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES ENABLE_EXPORTS TRUE)
set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

#=================== cimgui ===================
FetchContent_Declare(
    cimgui
    GIT_REPOSITORY https://github.com/cimgui/cimgui.git
    GIT_TAG        1.91.9bdock
    GIT_SUBMODULES ""
)
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()
FetchContent_Populate(cimgui)
FetchContent_GetProperties(ImGui)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_deps")
file(CREATE_LINK "${imgui_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_deps/imgui" SYMBOLIC)

add_library(cimgui STATIC ${cimgui_SOURCE_DIR}/cimgui.cpp)

target_include_directories(cimgui
    PRIVATE "${CMAKE_BINARY_DIR}/_deps"
    PUBLIC  "${cimgui_SOURCE_DIR}"
)

target_compile_definitions(cimgui
    INTERFACE CIMGUI_DEFINE_ENUMS_AND_STRUCTS
)

target_link_libraries(cimgui PUBLIC ImGui)
if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    target_link_libraries(${PROJECT_NAME} PRIVATE cimgui)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,cimgui>")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    get_property(IOS_TOOLCHAIN_FILE GLOBAL PROPERTY IOS_TOOLCHAIN_FILE)
    include(${IOS_TOOLCHAIN_FILE})
    set(BUNDLE_ID "dev.hm64.soh")
    set(SIGN_LIBRARY ON)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG;"
            "_CRT_SECURE_NO_WARNINGS;"
            "ENABLE_DX11;"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "INCLUDE_GAME_PRINTF;"
        "UNICODE;"
        "_UNICODE;"
        "_CRT_SECURE_NO_WARNINGS;"
        "_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS;"
    )
    add_compile_definitions(STORMLIB_NO_AUTO_LINK)
	set(STORMLIB_NO_AUTO_LINK ON)
elseif (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		"$<$<CONFIG:Debug>:"
			"_DEBUG"
		">"
		"$<$<CONFIG:Release>:"
			"NDEBUG"
		">"
        "SPDLOG_ACTIVE_LEVEL=3;"
		"SPDLOG_NO_THREAD_ID;"
        "SPDLOG_NO_TLS;"
		"STBI_NO_THREAD_LOCALS;"
	)
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang|AppleClang")
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		"$<$<CONFIG:Debug>:"
			"_DEBUG"
		">"
		"$<$<CONFIG:Release>:"
			"NDEBUG"
		">"
        "$<$<BOOL:${BUILD_CROWD_CONTROL}>:ENABLE_CROWD_CONTROL>"
		"SPDLOG_ACTIVE_LEVEL=0;"
		"_CONSOLE;"
		"_CRT_SECURE_NO_WARNINGS;"
		"UNICODE;"
		"_UNICODE"
	)
endif()


if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(ADDITIONAL_LIBRARY_DEPENDENCIES)
elseif(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    find_package(SDL2)
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        -lglad
        SDL2::SDL2
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "$<$<CONFIG:Debug>:-Wl,--wrap=abort>"
    )
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${DEVKITPRO}/portlibs/wiiu/include/
    )
else()
 	set(ADDITIONAL_LIBRARY_DEPENDENCIES)
endif()

if(USE_NETWORKING AND NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch" AND NOT CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
    FetchContent_Declare(
        ixwebsocket
        GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
        GIT_TAG        v11.4.5
    )
    set(USE_TLS ON CACHE BOOL "" FORCE)
    set(USE_MBED_TLS ON CACHE BOOL "" FORCE)
    set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(ixwebsocket)
    target_link_libraries(${PROJECT_NAME} PRIVATE ixwebsocket)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        USE_NETWORKING
        GHOSTSHIP_VERSION="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
    )
endif()

if(CMAKE_SYSTEM_NAME MATCHES "NintendoSwitch")
nx_generate_nacp(${PROJECT_NAME}.nacp
   NAME "${PROJECT_NAME}"
   AUTHOR "${PROJECT_TEAM}"
   VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
)

nx_create_nro(${PROJECT_NAME}
    NACP ${PROJECT_NAME}.nacp
    ICON ${CMAKE_CURRENT_SOURCE_DIR}/nx-logo.jpg
)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.nro DESTINATION . COMPONENT ${PROJECT_NAME})
else()
option(USE_STANDALONE "Build as a standalone executable" OFF)
option(BUILD_STORMLIB "Build with StormLib support" OFF)

option(BUILD_SM64 "Build with Super Mario 64 support" ON)
option(BUILD_MK64 "Build with Mario Kart 64 support" OFF)
option(BUILD_SF64 "Build with Star Fox 64 support" OFF)
option(BUILD_FZERO "Build with F-Zero X support" OFF)
option(BUILD_MARIO_ARTIST "Build with Mario Artist support" OFF)
add_subdirectory(Torch)
list(APPEND ADDITIONAL_LIBRARY_DEPENDENCIES torch)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")
################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /w;
            /Od
        >
        $<$<CONFIG:Release>:
            /Oi;
            /Gy;
            /W3
        >

        /wd4024 #different types for formal and actual parameter [x]. Usually seen along side 4047.
        /wd4047 #'uintptr_t' differs in levels of indirection from 'Gfx *'. Usually caused by passing a uintptr_t into a function expecting a pointer-types
        /wd4090 #'initializing': different 'const' qualifiers. Caused when initializing a non const struct member with a const char*.		
        /wd4101 # unreferenced local variable. Things like stack pads.
        /wd4133 #incompatible types. Usually caused by passing a c string into a function that takes a pointer.
        /wd4244 #conversion from [a] to [b]. Used to warn when copying to smaller type without a cast.
        /sdl-;
        /permissive-;
        /MP;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING}
    )
  target_compile_options(${PROJECT_NAME} PRIVATE  $<$<CONFIG:Debug>:/ZI;>)
  target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /INCREMENTAL
        >
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO;
            /FORCE:MULTIPLE
        >
        /MANIFEST:NO;
        /DEBUG;
        /SUBSYSTEM:WINDOWS
    )

  # Remove /RTC from msvc flags
  foreach (fentry
    CMAKE_C_FLAGS
    CMAKE_CXX_FLAGS
  )
    string (REGEX REPLACE "/RTC(su|[1su])" "" ${fentry} "${${fentry}}")
  endforeach(fentry)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
        target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall -Wextra -Wno-error
            -Wno-return-type
            -Wno-unused-parameter
            -Wno-unused-function
            -Wno-unused-variable
            -Wno-missing-field-initializers
            -Wno-parentheses
            -Wno-narrowing
            -Wno-missing-braces
            -Wno-int-conversion
            $<$<COMPILE_LANGUAGE:C>:
                -Werror-implicit-function-declaration
                -Wno-incompatible-pointer-types
                -fpermissive
            >
            $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
            $<$<COMPILE_LANGUAGE:CXX>:
                -Wno-c++11-narrowing
                -Wno-deprecated-enum-enum-conversion
            >
            -pthread
        )

        target_link_options(${PROJECT_NAME} PRIVATE
            -pthread
        )
    elseif (CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
      target_compile_options(${PROJECT_NAME} PRIVATE
          -Wall -Wextra -Wno-error
          -Wno-return-type
          -Wno-unused-parameter
          -Wno-unused-function
          -Wno-unused-variable
          -Wno-missing-field-initializers
          -Wno-parentheses
          -Wno-narrowing
          -Wno-missing-braces
          -Wno-int-conversion
          $<$<COMPILE_LANGUAGE:C>:
              -Wno-implicit-function-declaration
              -Wno-incompatible-pointer-types
              -fpermissive
          >
          $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
          $<$<COMPILE_LANGUAGE:CXX>:
              -Wno-c++11-narrowing
              -Wno-deprecated-enum-enum-conversion
          >
          -pthread
      )

      target_link_options(${PROJECT_NAME} PRIVATE
          -pthread
      )
  elseif (CMAKE_SYSTEM_NAME STREQUAL "CafeOS")
      target_compile_options(${PROJECT_NAME} PRIVATE
          -O2

          # disable some warnings to not clutter output
          -Wno-multichar
          -Wno-return-type
          -Wno-narrowing
          -Wno-switch-outside-range
          $<$<COMPILE_LANGUAGE:C>:
              -Werror-implicit-function-declaration
              -Wno-incompatible-pointer-types
              -Wno-discarded-array-qualifiers
              -Wno-discarded-qualifiers
              -Wno-int-conversion
              -Wno-builtin-declaration-mismatch
              -Wno-switch-unreachable
              -Wno-stringop-overflow
          >
      )
  else()
      if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        set(CPU_OPTION -msse2 -mfpmath=sse)
      elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(CPU_OPTION -mcpu=native)
      endif()

      target_compile_options(${PROJECT_NAME} PRIVATE
          -Wall -Wextra -Wno-error
          -Wno-unused-parameter
          -Wno-unused-function
          -Wno-unused-variable
          -Wno-missing-field-initializers
          -Wno-parentheses
          -Wno-narrowing
          -Wno-missing-braces
          -Wno-int-conversion
          $<$<COMPILE_LANGUAGE:C>:
              -Werror-implicit-function-declaration
              -Wno-incompatible-pointer-types
              -fpermissive
          >
          $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-enum-enum-conversion>
          -pthread
    ${CPU_OPTION}
      )

      target_link_options(${PROJECT_NAME} PRIVATE
          -pthread
          -Wl,-export-dynamic
      )
  endif()
endif()

add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMENT "Copying asset yamls..."
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/config.yml" "$<TARGET_FILE_DIR:Ghostship>/config.yml"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets/" "$<TARGET_FILE_DIR:Ghostship>/assets/"
)

include(${CMAKE_CURRENT_SOURCE_DIR}/libultraship/cmake/SetupTccRuntime.cmake)

set(PROPERTY_READER_GUARD_DISABLED TRUE)
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    lus_setup_tcc_runtime(${PROJECT_NAME} RESOURCES_DIR "../Resources/.tcc")
else()
    lus_setup_tcc_runtime(${PROJECT_NAME})
endif()
unset(PROPERTY_READER_GUARD_DISABLED)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
include(ExternalProject)
ExternalProject_Add(TorchExternal
	PREFIX TorchExternal
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/Torch
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/Torch
)
ExternalProject_Get_Property(TorchExternal install_dir)
get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (_is_multi_config)
  set(TORCH_EXECUTABLE ${install_dir}/src/TorchExternal-build/$<CONFIG>/torch)
else()
  set(TORCH_EXECUTABLE ${install_dir}/src/TorchExternal-build/torch)
endif()
add_custom_target(
    ExtractAssets
    DEPENDS TorchExternal
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${TORCH_EXECUTABLE} o2r baserom.us.z64 -u ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/sm64.o2r" "${CMAKE_BINARY_DIR}/sm64.o2r"
)

add_custom_target(
    GeneratePortO2R
    DEPENDS TorchExternal
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${TORCH_EXECUTABLE} pack port ghostship.o2r o2r -u ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/ghostship.o2r" "${CMAKE_BINARY_DIR}/ghostship.o2r"
)

find_program(CURL NAMES curl DOC "Path to the curl program.  Used to download files.")
execute_process(COMMAND ${CURL} -sSfL https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/master/gamecontrollerdb.txt -o ${CMAKE_BINARY_DIR}/gamecontrollerdb.txt OUTPUT_VARIABLE RESULT)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  install(FILES "${CMAKE_BINARY_DIR}/ghostship.o2r" DESTINATION . COMPONENT ${PROJECT_NAME})
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/" DESTINATION ./assets COMPONENT ${PROJECT_NAME})
  install(FILES "${CMAKE_BINARY_DIR}/config.yml" DESTINATION . COMPONENT ${PROJECT_NAME})
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ./debug COMPONENT ${PROJECT_NAME})
  install(FILES "${CMAKE_BINARY_DIR}/ghostship.o2r" DESTINATION . COMPONENT ${PROJECT_NAME})
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
add_custom_target(CreateOSXIcons
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset
    COMMAND sips -z 16 16     logo.png --out ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset/icon_16x16.png
    COMMAND sips -z 32 32     logo.png --out ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset/icon_16x16@2x.png
    COMMAND sips -z 32 32     logo.png --out ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset/icon_32x32.png
    COMMAND sips -z 64 64     logo.png --out ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset/icon_32x32@2x.png
    COMMAND sips -z 128 128   logo.png --out ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset/icon_128x128.png
    COMMAND sips -z 256 256   logo.png --out ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset/icon_128x128@2x.png
    COMMAND sips -z 256 256   logo.png --out ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset/icon_256x256.png
    COMMAND sips -z 512 512   logo.png --out ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset/icon_256x256@2x.png
    COMMAND sips -z 512 512   logo.png --out ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset/icon_512x512.png
    COMMAND cp                logo.png ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset/icon_512x512@2x.png
    COMMAND iconutil -c icns -o ${CMAKE_BINARY_DIR}/macosx/ghostship.icns ${CMAKE_BINARY_DIR}/macosx/ghostship.iconset
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Creating OSX icons ..."
    )
    add_dependencies(${PROJECT_NAME} CreateOSXIcons)
    configure_file("${CMAKE_SOURCE_DIR}/Info.plist" "${CMAKE_BINARY_DIR}/Info.plist" COPYONLY)
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ../MacOS COMPONENT Ghostship)
if(ENABLE_SCRIPTING)
    INSTALL(FILES "$<TARGET_FILE:libtcc>" DESTINATION ../MacOS COMPONENT Ghostship)
endif()
INSTALL(FILES ${CMAKE_BINARY_DIR}/gamecontrollerdb.txt DESTINATION ../MacOS COMPONENT Ghostship)
INSTALL(FILES ${CMAKE_BINARY_DIR}/ghostship.o2r DESTINATION ../Resources COMPONENT Ghostship)
INSTALL(FILES ${CMAKE_BINARY_DIR}/config.yml DESTINATION ../Resources COMPONENT Ghostship)
INSTALL(DIRECTORY ${CMAKE_BINARY_DIR}/assets/ DESTINATION ../Resources/assets COMPONENT Ghostship)

# Fix bundle to include and relink all dependencies
install(CODE "
    include(BundleUtilities)
    fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/../MacOS/Ghostship\" \"\" \"${dirs}\")
")
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY APPIMAGE_DESKTOP_FILE_TERMINAL YES)
set_property(TARGET ${PROJECT_NAME} PROPERTY APPIMAGE_DESKTOP_FILE "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.desktop")
set_property(TARGET ${PROJECT_NAME} PROPERTY APPIMAGE_ICON_FILE "${CMAKE_SOURCE_DIR}/logo.png")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CPACK_GENERATOR "External")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows|NintendoSwitch|CafeOS")
    set(CPACK_GENERATOR "ZIP")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CPACK_GENERATOR "Bundle")
endif()

set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_SOURCE_DIR}/cmake/configure-packaging.cmake)
include(cmake/packaging.cmake)