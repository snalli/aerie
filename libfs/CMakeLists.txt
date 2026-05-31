cmake_minimum_required(VERSION 3.16)
project(aerie LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# Build options  (override on command line: cmake -DRPC=fast -B build)
# ---------------------------------------------------------------------------
option(BUILD_BENCH "Build benchmark programs" ON)
option(COVERAGE   "Build with gcov code-coverage instrumentation" OFF)

set(RPC "net" CACHE STRING "RPC backend: net | fast | fast-two")
set_property(CACHE RPC PROPERTY STRINGS net fast fast-two)

set(SCMPOOL "user" CACHE STRING "SCM pool implementation: kernel | user")
set_property(CACHE SCMPOOL PROPERTY STRINGS kernel user)

set(GOOGLE_SPARSEHASH "" CACHE PATH "Path to google-sparsehash headers (if not on system include path)")
set(LIBCONFIG_ROOT    "" CACHE PATH "Root of a locally-built libconfig source tree (optional; uses system install by default)")
set(LIBHOARD          "" CACHE PATH "Path to libhoard directory (optional)")

# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------
add_compile_options(-O2 -Wall -Wextra -Werror)

# Code-coverage instrumentation (gcov/lcov). Enable with -DCOVERAGE=ON.
if(COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()
add_compile_definitions(__STDC_FORMAT_MACROS)

# RPC mode
if(RPC STREQUAL "fast")
    add_compile_definitions(_CLT2SVR_RPCFAST _SVR2CLT_RPCNET)
elseif(RPC STREQUAL "fast-two")
    add_compile_definitions(_CLT2SVR_RPCFAST _SVR2CLT_RPCFAST)
else()                          # net (default)
    add_compile_definitions(_CLT2SVR_RPCNET _SVR2CLT_RPCNET)
endif()

# SCM pool mode
if(SCMPOOL STREQUAL "user")
    add_compile_definitions(_SCM_POOL_USERMODE)
else()
    add_compile_definitions(_SCM_POOL_KERNELMODE)
endif()

add_compile_definitions(KVFS_USE_INDIRECT)

# ---------------------------------------------------------------------------
# Platform libs (rt merged into glibc on modern Linux, but harmless to list)
# ---------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    set(RT_LIB rt)
endif()

# ---------------------------------------------------------------------------
# External dependencies
# ---------------------------------------------------------------------------
find_package(Threads REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(LibConfig REQUIRED)

if(GOOGLE_SPARSEHASH)
    include_directories(${GOOGLE_SPARSEHASH})
endif()

if(LIBHOARD)
    link_directories(${LIBHOARD})
    add_link_options(-lhoard)
endif()

# ---------------------------------------------------------------------------
# Common INTERFACE target — provides the src/ include root AND libconfig
# headers to every target in the project.  libconfig.h is transitively
# pulled in by cdebug.h → rtcconfig.h so all targets need it, not just bcs.
# ---------------------------------------------------------------------------
add_library(aerie_includes INTERFACE)
target_include_directories(aerie_includes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(aerie_includes INTERFACE
    LibConfig::LibConfig
    LibConfig::LibConfigPP
)

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------
add_subdirectory(src)

if(BUILD_BENCH)
    add_subdirectory(bench)
endif()
