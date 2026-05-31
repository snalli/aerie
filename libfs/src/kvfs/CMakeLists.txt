# ---------------------------------------------------------------------------
# kvfs – Key-value store filesystem
#
# Library architecture:
#
#   KVFS Filesystem Layer:
#     kvfsc (client library - PUBLIC API)
#     kvfss (server library - PUBLIC API)
#
#   Executables:
#     kvfs_server / kvfs_mkfs
#
# Dependency graph (bottom → top):
#   kvfsc / kvfss → [osd*, bcs*, scm, common]
#   kvfs_* executables → [kvfsc/kvfss, ...]
# ---------------------------------------------------------------------------

# ============================================================================
# KVFS Public Libraries
# ============================================================================

# ---------- kvfsc (KVFS client library) ----------------------------------
# PUBLIC API: Applications link against this for KVFS client functionality
#
# Public headers (via include/kvfs/):
#   - include/kvfs/c_api.h: C API for key-value operations (put, get, delete)
#
# Implementation-only headers (not for external use):
#   - client/client.h, client/table.h, client/sb.h, etc.
#
add_library(kvfsc SHARED
    client/c_api.cc
    client/client.cc
    client/sb.cc
    client/table.cc
)

# Public interface: expose public headers via include/ directory
target_include_directories(kvfsc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/client>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(kvfsc PUBLIC aerie_includes)

# Private implementation dependencies (not exposed to users)
target_link_libraries(kvfsc PRIVATE osdclt bcsclt scm common)

# Version information
set_target_properties(kvfsc PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

# Install rules
install(TARGETS kvfsc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES ${CMAKE_SOURCE_DIR}/include/kvfs/c_api.h DESTINATION include/kvfs)

# ---------- kvfss (KVFS server library) ----------------------------------
# PUBLIC API: Applications link against this for KVFS server functionality
#
# Public headers (via include/kvfs/):
#   - include/kvfs/fs.h: Server filesystem interface
#
# Implementation-only headers (not for external use):
#   - server/server.h, server/session.h, server/subtable.h, etc.
#
add_library(kvfss SHARED
    server/fs.cc
    server/publisher.cc
    server/server.cc
    server/session.cc
    server/subtable.cc
)

# Public interface: expose public headers via include/ directory
target_include_directories(kvfss PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(kvfss PUBLIC aerie_includes)

# Private implementation dependencies (not exposed to users)
target_link_libraries(kvfss PRIVATE osdsrv bcssrv scm common)

# Version information
set_target_properties(kvfss PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

# Install rules
install(TARGETS kvfss
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES ${CMAKE_SOURCE_DIR}/include/kvfs/fs.h DESTINATION include/kvfs)

# ============================================================================
# KVFS Executables
# ============================================================================

# ---------- kvfs_server (KVFS server executable) -------------------------
add_executable(kvfs_server server/main.cc)
target_link_libraries(kvfs_server PRIVATE kvfss bcssrv osdsrv scm common Threads::Threads ${RT_LIB})

# ---------- kvfs_mkfs (KVFS filesystem formatting tool) ------------------
add_executable(kvfs_mkfs
    tool/main.cc
    tool/mkfs.cc
)
target_link_libraries(kvfs_mkfs PRIVATE kvfss bcssrv osdsrv scm common Threads::Threads ${RT_LIB})
