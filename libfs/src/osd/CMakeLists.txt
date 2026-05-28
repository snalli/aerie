# ---------------------------------------------------------------------------
# OSD – Object Storage Device layer
# ---------------------------------------------------------------------------

add_library(osdclt OBJECT
    main/client/journal.cc
    main/client/hlckmgr.cc
    main/client/lckmgr.cc
    main/client/omgr.cc
    main/client/salloc.cc
    main/client/stm.cc
    main/client/stsystem.cc
    main/client/registry.cc
    containers/name/container-client.cc
    containers/super/container.cc
    containers/byte/container.cc
    containers/needle/container.cc
)
target_link_libraries(osdclt PUBLIC aerie_includes)

add_library(osdsrv OBJECT
    main/server/journal.cc
    main/server/hlckmgr.cc
    main/server/lckmgr.cc
    main/server/publisher.cc
    main/server/registry.cc
    main/server/salloc.cc
    main/server/shbuf.cc
    main/server/session.cc
    main/server/verifier.cc
    containers/name/container-server.cc
)
target_link_libraries(osdsrv PUBLIC aerie_includes)
