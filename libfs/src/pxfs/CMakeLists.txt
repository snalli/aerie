# ---------------------------------------------------------------------------
# pxfs – POSIX-compliant filesystem
#
# Library architecture:
#
#   Metadata Layer:
#     mfsclt (client) / mfssrv (server)
#
#   PXFS Filesystem Layer:
#     pxfsc (client library - PUBLIC API)
#     pxfss (server library - PUBLIC API)
#
#   Executables:
#     pxfs_server / pxfs_client / pxfs_mkfs
#
# Dependency graph (bottom → top):
#   pxfsc / pxfss → [mfsclt/mfssrv, osd*, bcs*, scm, common]
#   pxfs_* executables → [pxfsc/pxfss, ...]
# ---------------------------------------------------------------------------

# ============================================================================
# LAYER 1: Metadata Filesystem (Internal)
# ============================================================================

# ---------- mfsclt (metadata filesystem client) ----------------------------
add_library(mfsclt SHARED
    mfs/client/dir_inode.cc
    mfs/client/file_inode.cc
    mfs/client/inode_factory.cc
    mfs/client/mfs.cc
    mfs/client/sb_factory.cc
)
target_link_libraries(mfsclt PUBLIC aerie_includes)

# ---------- mfssrv (metadata filesystem server) ----------------------------
add_library(mfssrv SHARED
    mfs/server/dir_inode.cc
    mfs/server/mfs.cc
    mfs/server/publisher.cc
)
target_link_libraries(mfssrv PUBLIC aerie_includes)

# ============================================================================
# LAYER 2: PXFS Public Libraries
# ============================================================================

# ---------- pxfsc (PXFS client library) -----------------------------------
# PUBLIC API: Applications link against this for PXFS client functionality
#
# Public headers (via include/pxfs/):
#   - include/pxfs/c_api.h: C API for filesystem operations
#
# Implementation-only headers (not for external use):
#   - client/client.h, client/file.h, client/inode.h, etc.
#
add_library(pxfsc SHARED
    client/c_api.cc
    client/client.cc
    client/file.cc
    client/fsomgr.cc
    client/inode.cc
    client/mpinode.cc
    client/namespace.cc
)

# Public interface: expose public headers via include/ directory
target_include_directories(pxfsc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/client>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(pxfsc PUBLIC aerie_includes)

# Private implementation dependencies (not exposed to users)
target_link_libraries(pxfsc PRIVATE mfsclt osdclt bcsclt scm common)

# Version information
set_target_properties(pxfsc PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

# Install rules
install(TARGETS pxfsc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES ${CMAKE_SOURCE_DIR}/include/pxfs/c_api.h DESTINATION include/pxfs)

# ---------- pxfss (PXFS server library) -----------------------------------
# PUBLIC API: Applications link against this for PXFS server functionality
#
# Public headers (via include/pxfs/):
#   - include/pxfs/fs.h: Server filesystem interface
#
# Implementation-only headers (not for external use):
#   - server/server.h, server/session.h, etc.
#
add_library(pxfss SHARED
    server/fs.cc
    server/server.cc
    server/session.cc
)

# Public interface: expose public headers via include/ directory
target_include_directories(pxfss PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(pxfss PUBLIC aerie_includes)

# Private implementation dependencies (not exposed to users)
target_link_libraries(pxfss PRIVATE osdsrv bcssrv mfssrv scm common)

# Version information
set_target_properties(pxfss PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

# Install rules
install(TARGETS pxfss
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES ${CMAKE_SOURCE_DIR}/include/pxfs/fs.h DESTINATION include/pxfs)

# ============================================================================
# LAYER 3: PXFS Executables
# ============================================================================

# ---------- pxfs_server (PXFS server executable) ---------------------------
add_executable(pxfs_server server/main.cc)
target_link_libraries(pxfs_server PRIVATE pxfss bcssrv osdsrv mfssrv scm common Threads::Threads ${RT_LIB})

# ---------- pxfs_client (PXFS debug/test client) --------------------------
add_executable(pxfs_client client/main.cc)
target_link_libraries(pxfs_client PRIVATE pxfsc mfsclt bcsclt osdclt scm common Threads::Threads ${RT_LIB})

# ---------- pxfs_mkfs (PXFS filesystem formatting tool) -------------------
add_executable(pxfs_mkfs
    tool/main.cc
    tool/mkfs.cc
)
target_link_libraries(pxfs_mkfs PRIVATE pxfss bcssrv osdsrv mfssrv scm common Threads::Threads ${RT_LIB})
