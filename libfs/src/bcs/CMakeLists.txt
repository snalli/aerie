# ---------------------------------------------------------------------------
# BCS – Basic Communication Services
# ---------------------------------------------------------------------------

add_library(rpcnet OBJECT
    backend/rpc-net/rpc.cc
    backend/rpc-net/connection.cc
    backend/rpc-net/pollmgr.cc
    backend/rpc-net/thr_pool.cc
    backend/rpc-net/jsl_log.cc
)
target_link_libraries(rpcnet PUBLIC aerie_includes Threads::Threads)

add_library(rpcfast OBJECT
    backend/rpc-fast/rpc.cc
)
target_link_libraries(rpcfast PUBLIC aerie_includes Threads::Threads)

add_library(bcs_common OBJECT
    main/common/rtconfig.cc
    main/common/debug.cc
    main/common/cdebug.c
    main/common/rtcconfig.c
)
target_link_libraries(bcs_common PUBLIC aerie_includes)

add_library(bcsclt OBJECT
    main/client/ipc.cc
    main/client/shbuf.cc
)
target_link_libraries(bcsclt PUBLIC aerie_includes)

add_library(bcssrv OBJECT
    main/server/cltdsc.cc
    main/server/ipc.cc
    main/server/sessionmgr.cc
    main/server/session.cc
    main/server/shbuf.cc
    main/server/shbufmgr.cc
)
target_link_libraries(bcssrv PUBLIC aerie_includes)
