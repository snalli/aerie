# ---------------------------------------------------------------------------
# BCS – Basic Communication Services
#
# Layer order:
#   rpcnet / rpcfast        raw RPC transports
#   bcs_common (OBJECT)     shared debug/config objects compiled once
#   bcsclt                  client-side IPC
#   bcssrv                  server-side IPC
# ---------------------------------------------------------------------------

# ---------- rpc-net ---------------------------------------------------------
add_library(rpcnet SHARED
    backend/rpc-net/rpc.cc
    backend/rpc-net/connection.cc
    backend/rpc-net/pollmgr.cc
    backend/rpc-net/thr_pool.cc
    backend/rpc-net/jsl_log.cc
)
target_link_libraries(rpcnet PUBLIC aerie_includes Threads::Threads)

# ---------- rpc-fast --------------------------------------------------------
add_library(rpcfast SHARED
    backend/rpc-fast/rpc.cc
)
target_link_libraries(rpcfast PUBLIC aerie_includes Threads::Threads)

# ---------- bcs common (compiled once, linked into both clt and srv) --------
add_library(bcs_common OBJECT
    main/common/rtconfig.cc
    main/common/debug.cc
    main/common/cdebug.c
    main/common/rtcconfig.c
)
target_link_libraries(bcs_common PUBLIC aerie_includes)

# ---------- bcsclt ----------------------------------------------------------
add_library(bcsclt SHARED
    main/client/ipc.cc
    main/client/shbuf.cc
    $<TARGET_OBJECTS:bcs_common>
)
target_link_libraries(bcsclt PUBLIC aerie_includes rpcnet rpcfast)

# ---------- bcssrv ----------------------------------------------------------
add_library(bcssrv SHARED
    main/server/cltdsc.cc
    main/server/ipc.cc
    main/server/sessionmgr.cc
    main/server/session.cc
    main/server/shbuf.cc
    main/server/shbufmgr.cc
    $<TARGET_OBJECTS:bcs_common>
)
target_link_libraries(bcssrv PUBLIC aerie_includes rpcnet rpcfast)
