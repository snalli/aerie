# FindLibConfig.cmake
#
# Finds libconfig and libconfig++.
# If LIBCONFIG_ROOT is set, searches there first (for locally-built source trees
# where the .libs/ sub-directory holds the built shared objects).
#
# Imported targets created:
#   LibConfig::LibConfig    – the C library  (-lconfig)
#   LibConfig::LibConfigPP  – the C++ library (-lconfig++)
#
# Also sets:
#   LibConfig_FOUND
#   LIBCONFIG_INCLUDE_DIR
#   LIBCONFIG_LIBRARY
#   LIBCONFIGPP_LIBRARY

find_path(LIBCONFIG_INCLUDE_DIR
    NAMES libconfig.h libconfig.hh
    HINTS ${LIBCONFIG_ROOT}
    PATH_SUFFIXES include
)

find_library(LIBCONFIG_LIBRARY
    NAMES config
    HINTS ${LIBCONFIG_ROOT}
    PATH_SUFFIXES .libs lib lib64
)

find_library(LIBCONFIGPP_LIBRARY
    NAMES config++
    HINTS ${LIBCONFIG_ROOT}
    PATH_SUFFIXES .libs lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibConfig
    REQUIRED_VARS LIBCONFIG_LIBRARY LIBCONFIGPP_LIBRARY LIBCONFIG_INCLUDE_DIR
)

if(LibConfig_FOUND)
    if(NOT TARGET LibConfig::LibConfig)
        add_library(LibConfig::LibConfig UNKNOWN IMPORTED)
        set_target_properties(LibConfig::LibConfig PROPERTIES
            IMPORTED_LOCATION             "${LIBCONFIG_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBCONFIG_INCLUDE_DIR}"
        )
    endif()

    if(NOT TARGET LibConfig::LibConfigPP)
        add_library(LibConfig::LibConfigPP UNKNOWN IMPORTED)
        set_target_properties(LibConfig::LibConfigPP PROPERTIES
            IMPORTED_LOCATION             "${LIBCONFIGPP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBCONFIG_INCLUDE_DIR}"
        )
    endif()
endif()
