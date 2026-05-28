# ---------------------------------------------------------------------------
# pxfs – POSIX-compliant filesystem
#
# Dependency graph (bottom → top):
#   mfsclt / mfssrv   metadata filesystem backend
#   fsc               client library
#   fss               server library
#   pxfs_server / pxfs_client / pxfs_mkfs   executables
# ---------------------------------------------------------------------------

# ---------- mfsclt ----------------------------------------------------------
add_library(mfsclt SHARED
    mfs/client/dir_inode.cc
    mfs/client/file_inode.cc
    mfs/client/inode_factory.cc
    mfs/client/mfs.cc
    mfs/client/sb_factory.cc
)
target_link_libraries(mfsclt PUBLIC aerie_includes)

# ---------- mfssrv ----------------------------------------------------------
add_library(mfssrv SHARED
    mfs/server/dir_inode.cc
    mfs/server/mfs.cc
    mfs/server/publisher.cc
)
target_link_libraries(mfssrv PUBLIC aerie_includes)

# ---------- fsc (pxfs client library) ---------------------------------------
add_library(fsc SHARED
    client/c_api.cc
    client/client.cc
    client/file.cc
    client/fsomgr.cc
    client/inode.cc
    client/mpinode.cc
    client/namespace.cc
)
target_link_libraries(fsc PUBLIC aerie_includes mfsclt osdclt bcsclt scm common)

# ---------- fss (pxfs server library) ---------------------------------------
add_library(fss SHARED
    server/fs.cc
    server/server.cc
    server/session.cc
)
target_link_libraries(fss PUBLIC aerie_includes osdsrv bcssrv mfssrv scm common)

# ---------- pxfs_server -----------------------------------------------------
add_executable(pxfs_server server/main.cc)
target_link_libraries(pxfs_server PRIVATE fss bcssrv osdsrv mfssrv scm common Threads::Threads ${RT_LIB})

# ---------- pxfs_client (debug/test client) ---------------------------------
add_executable(pxfs_client client/main.cc)
target_link_libraries(pxfs_client PRIVATE fsc mfsclt bcsclt osdclt scm common Threads::Threads ${RT_LIB})

# ---------- pxfs_mkfs (filesystem formatting tool) --------------------------
add_executable(pxfs_mkfs
    tool/main.cc
    tool/mkfs.cc
)
target_link_libraries(pxfs_mkfs PRIVATE fss bcssrv osdsrv mfssrv scm common Threads::Threads ${RT_LIB})
