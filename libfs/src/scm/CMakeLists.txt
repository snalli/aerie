# ---------------------------------------------------------------------------
# SCM – Storage Class Memory abstraction
#
# Pool backend is selected at configure time via -DSCMPOOL=kernel|user.
# The compile definition (_SCM_POOL_KERNELMODE / _SCM_POOL_USERMODE) is
# set globally in the root CMakeLists.txt.
# ---------------------------------------------------------------------------

set(scm_core_sources
    scm/model.cc
    scm/memcpy.cc
    pregion/pregion.cc
    pheap/pheap.cc
    pheap/vistaheap.c
)

if(SCMPOOL STREQUAL "user")
    set(pool_sources pool/user/pool.cc)
else()                          # kernel (default)
    set(pool_sources
        pool/kernel/pool-vistaheap.cc
        pool/kernel/buddy.cc
    )
endif()

add_library(scm SHARED ${scm_core_sources} ${pool_sources})
target_link_libraries(scm PUBLIC aerie_includes Threads::Threads)

# ---------------------------------------------------------------------------
# pool_tool – offline storage-pool creation/inspection utility
# ---------------------------------------------------------------------------
add_executable(pool_tool
    tool/pool/main.cc
    tool/pool/mkpool.cc
)
target_link_libraries(pool_tool PRIVATE scm Threads::Threads)
