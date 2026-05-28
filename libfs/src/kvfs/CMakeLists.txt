# ---------------------------------------------------------------------------
# kvfs – Key-value store filesystem (object units only; executables in parent)
# ---------------------------------------------------------------------------

add_library(kvfsc OBJECT
    client/c_api.cc
    client/client.cc
    client/sb.cc
    client/table.cc
)
target_link_libraries(kvfsc PUBLIC aerie_includes)

add_library(kvfss OBJECT
    server/fs.cc
    server/publisher.cc
    server/server.cc
    server/session.cc
    server/subtable.cc
)
target_link_libraries(kvfss PUBLIC aerie_includes)
