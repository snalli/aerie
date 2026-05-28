# ---------------------------------------------------------------------------
# pxfs – POSIX-compliant filesystem (object units only; executables in parent)
# ---------------------------------------------------------------------------

add_library(mfsclt OBJECT
    mfs/client/dir_inode.cc
    mfs/client/file_inode.cc
    mfs/client/inode_factory.cc
    mfs/client/mfs.cc
    mfs/client/sb_factory.cc
)
target_link_libraries(mfsclt PUBLIC aerie_includes)

add_library(mfssrv OBJECT
    mfs/server/dir_inode.cc
    mfs/server/mfs.cc
    mfs/server/publisher.cc
)
target_link_libraries(mfssrv PUBLIC aerie_includes)

add_library(fsc OBJECT
    client/c_api.cc
    client/client.cc
    client/file.cc
    client/fsomgr.cc
    client/inode.cc
    client/mpinode.cc
    client/namespace.cc
)
target_link_libraries(fsc PUBLIC aerie_includes)

add_library(fss OBJECT
    server/fs.cc
    server/server.cc
    server/session.cc
)
target_link_libraries(fss PUBLIC aerie_includes)
