# ---------------------------------------------------------------------------
# rxfs – Read-only / replica filesystem  (client-side only)
# ---------------------------------------------------------------------------

add_library(rxfsc SHARED
    client/c_api.cc
    client/client.cc
    client/dir_inode.cc
    client/file.cc
    client/namespace.cc
)
target_link_libraries(rxfsc PUBLIC aerie_includes osdclt bcsclt scm common)
