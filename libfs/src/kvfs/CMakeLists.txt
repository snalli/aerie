# ---------------------------------------------------------------------------
# kvfs – Key-value store filesystem
# ---------------------------------------------------------------------------

# ---------- kvfsc (client library) ------------------------------------------
add_library(kvfsc SHARED
    client/c_api.cc
    client/client.cc
    client/sb.cc
    client/table.cc
)
target_link_libraries(kvfsc PUBLIC aerie_includes osdclt bcsclt scm common)

# ---------- kvfss (server library) ------------------------------------------
add_library(kvfss SHARED
    server/fs.cc
    server/publisher.cc
    server/server.cc
    server/session.cc
    server/subtable.cc
)
target_link_libraries(kvfss PUBLIC aerie_includes osdsrv bcssrv scm common)

# ---------- kvfs_server -----------------------------------------------------
add_executable(kvfs_server server/main.cc)
target_link_libraries(kvfs_server PRIVATE kvfss bcssrv osdsrv scm common Threads::Threads ${RT_LIB})

# ---------- kvfs_mkfs -------------------------------------------------------
add_executable(kvfs_mkfs
    tool/main.cc
    tool/mkfs.cc
)
target_link_libraries(kvfs_mkfs PRIVATE kvfss bcssrv osdsrv scm common Threads::Threads ${RT_LIB})
