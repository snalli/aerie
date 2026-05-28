# ---------------------------------------------------------------------------
# Process all subsystem object-library targets first
# ---------------------------------------------------------------------------
add_subdirectory(bcs)
add_subdirectory(scm)
add_subdirectory(common)
add_subdirectory(osd)
add_subdirectory(pxfs)
add_subdirectory(cfs)
add_subdirectory(rxfs)
add_subdirectory(kvfs)

# ---------------------------------------------------------------------------
# libfs – single unified shared library
#
# All 18 object targets are folded into one .so so callers only need -lfs.
# ---------------------------------------------------------------------------
add_library(fs SHARED
    $<TARGET_OBJECTS:rpcnet>
    $<TARGET_OBJECTS:rpcfast>
    $<TARGET_OBJECTS:bcs_common>
    $<TARGET_OBJECTS:bcsclt>
    $<TARGET_OBJECTS:bcssrv>
    $<TARGET_OBJECTS:scm>
    $<TARGET_OBJECTS:common>
    $<TARGET_OBJECTS:osdclt>
    $<TARGET_OBJECTS:osdsrv>
    $<TARGET_OBJECTS:mfsclt>
    $<TARGET_OBJECTS:mfssrv>
    $<TARGET_OBJECTS:fsc>
    $<TARGET_OBJECTS:fss>
    $<TARGET_OBJECTS:cfsc>
    $<TARGET_OBJECTS:cfss>
    $<TARGET_OBJECTS:kvfsc>
    $<TARGET_OBJECTS:kvfss>
    $<TARGET_OBJECTS:rxfsc>
)
target_link_libraries(fs PUBLIC aerie_includes Threads::Threads)
if(RT_LIB)
    target_link_libraries(fs PUBLIC ${RT_LIB})
endif()

# ---------------------------------------------------------------------------
# Executables — all link only against fs
# ---------------------------------------------------------------------------

# --- SCM pool tool ----------------------------------------------------------
add_executable(pool_tool
    scm/tool/pool/main.cc
    scm/tool/pool/mkpool.cc
)
target_link_libraries(pool_tool PRIVATE fs Threads::Threads)

# --- pxfs -------------------------------------------------------------------
add_executable(pxfs_server  pxfs/server/main.cc)
target_link_libraries(pxfs_server  PRIVATE fs Threads::Threads)

add_executable(pxfs_client  pxfs/client/main.cc)
target_link_libraries(pxfs_client  PRIVATE fs Threads::Threads)

add_executable(pxfs_mkfs    pxfs/tool/main.cc pxfs/tool/mkfs.cc)
target_link_libraries(pxfs_mkfs    PRIVATE fs Threads::Threads)

# --- cfs --------------------------------------------------------------------
add_executable(cfs_server   cfs/server/main.cc)
target_link_libraries(cfs_server   PRIVATE fs Threads::Threads)

add_executable(cfs_client   cfs/client/main.cc)
target_link_libraries(cfs_client   PRIVATE fs Threads::Threads)

add_executable(cfs_mkfs     cfs/tool/main.cc cfs/tool/mkfs.cc)
target_link_libraries(cfs_mkfs     PRIVATE fs Threads::Threads)

# --- kvfs -------------------------------------------------------------------
add_executable(kvfs_server  kvfs/server/main.cc)
target_link_libraries(kvfs_server  PRIVATE fs Threads::Threads)

add_executable(kvfs_mkfs    kvfs/tool/main.cc kvfs/tool/mkfs.cc)
target_link_libraries(kvfs_mkfs    PRIVATE fs Threads::Threads)
