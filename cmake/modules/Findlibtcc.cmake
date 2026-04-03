# Findlibtcc.cmake
# -------------
# Finds the Tiny C Compiler (libtcc) library and header.
#
# Defines the following variables:
#   libtcc_FOUND        - True if libtcc was found
#   libtcc_INCLUDE_DIRS - Include directories for libtcc
#   libtcc_LIBRARIES    - Libraries to link against libtcc
#
# Provides the following imported target:
#   libtcc              - The libtcc library target

# Search for the header
find_path(libtcc_INCLUDE_DIR
    NAMES libtcc.h
    DOC "Path to libtcc include directory"
)

# Search for the library (prioritizing 'libtcc' over 'tcc')
find_library(libtcc_LIBRARY
    NAMES libtcc tcc
    DOC "Path to libtcc library"
)

# Handle standard arguments (REQUIRED, QUIET, etc.)
include(FindPackageHandleStandardArgs)
# THIS MUST EXACTLY MATCH your find_package() call case
find_package_handle_standard_args(libtcc
    REQUIRED_VARS libtcc_LIBRARY libtcc_INCLUDE_DIR
)

# Set variables and create the imported target if found
if(libtcc_FOUND)
    set(libtcc_INCLUDE_DIRS ${libtcc_INCLUDE_DIR})
    set(libtcc_LIBRARIES ${libtcc_LIBRARY})

    if(NOT TARGET libtcc)
        add_library(libtcc UNKNOWN IMPORTED)
        set_target_properties(libtcc PROPERTIES
            IMPORTED_LOCATION "${libtcc_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${libtcc_INCLUDE_DIR}"
        )
    endif()
endif()

# Hide these from the standard CMake GUI
mark_as_advanced(libtcc_INCLUDE_DIR libtcc_LIBRARY)