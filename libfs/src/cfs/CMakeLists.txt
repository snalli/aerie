# ---------------------------------------------------------------------------
# cfs – Cache filesystem
# ---------------------------------------------------------------------------

# ---------- cfsc (client library) -------------------------------------------
add_library(cfsc SHARED
    client/c_api.cc
    client/client.cc
    client/file.cc
)
target_link_libraries(cfsc PUBLIC aerie_includes osdclt bcsclt scm common)

# ---------- cfss (server library) -------------------------------------------
add_library(cfss SHARED
    server/dir_inode.cc
    server/fs.cc
    server/namespace.cc
    server/server.cc
    server/session.cc
)
target_link_libraries(cfss PUBLIC aerie_includes osdsrv bcssrv scm common)

# ---------- cfs_server ------------------------------------------------------
add_executable(cfs_server server/main.cc)
target_link_libraries(cfs_server PRIVATE cfss bcssrv osdsrv scm common Threads::Threads ${RT_LIB})

# ---------- cfs_client (debug/test client) ----------------------------------
add_executable(cfs_client client/main.cc)
target_link_libraries(cfs_client PRIVATE cfsc bcsclt osdclt scm common Threads::Threads ${RT_LIB})

# ---------- cfs_mkfs --------------------------------------------------------
# Note: the mkfs tool also depends on mfssrv (same as pxfs_mkfs)
add_executable(cfs_mkfs
    tool/main.cc
    tool/mkfs.cc
)
target_link_libraries(cfs_mkfs PRIVATE cfss bcssrv osdsrv mfssrv scm common Threads::Threads ${RT_LIB})
