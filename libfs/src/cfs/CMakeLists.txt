# ---------------------------------------------------------------------------
# cfs – Cache filesystem
#
# Library architecture:
#
#   CFS Filesystem Layer:
#     cfsc (client library - PUBLIC API)
#     cfss (server library - PUBLIC API)
#
#   Executables:
#     cfs_server / cfs_client / cfs_mkfs
#
# Dependency graph (bottom → top):
#   cfsc / cfss → [osd*, bcs*, scm, common]
#   cfs_* executables → [cfsc/cfss, ...]
# ---------------------------------------------------------------------------

# ============================================================================
# CFS Public Libraries
# ============================================================================

# ---------- cfsc (CFS client library) -----------------------------------
# PUBLIC API: Applications link against this for CFS client functionality
#
# Public headers (via include/cfs/):
#   - include/cfs/c_api.h: C API for filesystem operations
#
# Implementation-only headers (not for external use):
#   - client/client.h, client/file.h, etc.
#
add_library(cfsc SHARED
    client/c_api.cc
    client/client.cc
    client/file.cc
)

# Public interface: expose public headers via include/ directory
target_include_directories(cfsc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/client>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(cfsc PUBLIC aerie_includes)

# Private implementation dependencies (not exposed to users)
target_link_libraries(cfsc PRIVATE osdclt bcsclt scm common)

# Version information
set_target_properties(cfsc PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

# Install rules
install(TARGETS cfsc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES ${CMAKE_SOURCE_DIR}/include/cfs/c_api.h DESTINATION include/cfs)

# ---------- cfss (CFS server library) -----------------------------------
# PUBLIC API: Applications link against this for CFS server functionality
#
# Public headers (via include/cfs/):
#   - include/cfs/fs.h: Server filesystem interface
#
# Implementation-only headers (not for external use):
#   - server/server.h, server/session.h, server/namespace.h, etc.
#
add_library(cfss SHARED
    server/dir_inode.cc
    server/fs.cc
    server/namespace.cc
    server/server.cc
    server/session.cc
)

# Public interface: expose public headers via include/ directory
target_include_directories(cfss PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(cfss PUBLIC aerie_includes)

# Private implementation dependencies (not exposed to users)
target_link_libraries(cfss PRIVATE osdsrv bcssrv scm common)

# Version information
set_target_properties(cfss PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

# Install rules
install(TARGETS cfss
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES ${CMAKE_SOURCE_DIR}/include/cfs/fs.h DESTINATION include/cfs)

# ============================================================================
# CFS Executables
# ============================================================================

# ---------- cfs_server (CFS server executable) -------------------------
add_executable(cfs_server server/main.cc)
target_link_libraries(cfs_server PRIVATE cfss bcssrv osdsrv scm common Threads::Threads ${RT_LIB})

# ---------- cfs_client (CFS debug/test client) -------------------------
add_executable(cfs_client client/main.cc)
target_link_libraries(cfs_client PRIVATE cfsc bcsclt osdclt scm common Threads::Threads ${RT_LIB})

# ---------- cfs_mkfs (CFS filesystem formatting tool) -------------------
# Note: the mkfs tool also depends on mfssrv (same as pxfs_mkfs)
add_executable(cfs_mkfs
    tool/main.cc
    tool/mkfs.cc
)
target_link_libraries(cfs_mkfs PRIVATE cfss bcssrv osdsrv mfssrv scm common Threads::Threads ${RT_LIB})
