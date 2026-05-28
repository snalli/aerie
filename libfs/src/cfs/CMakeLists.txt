# ---------------------------------------------------------------------------
# cfs – Cache filesystem (object units only; executables in parent)
# ---------------------------------------------------------------------------

add_library(cfsc OBJECT
    client/c_api.cc
    client/client.cc
    client/file.cc
)
target_link_libraries(cfsc PUBLIC aerie_includes)

add_library(cfss OBJECT
    server/dir_inode.cc
    server/fs.cc
    server/namespace.cc
    server/server.cc
    server/session.cc
)
target_link_libraries(cfss PUBLIC aerie_includes)
