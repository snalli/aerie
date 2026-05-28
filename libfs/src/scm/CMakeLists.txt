# ---------------------------------------------------------------------------
# SCM – Storage Class Memory abstraction
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
else()
    set(pool_sources
        pool/kernel/pool-vistaheap.cc
        pool/kernel/buddy.cc
    )
endif()

add_library(scm OBJECT ${scm_core_sources} ${pool_sources})
target_link_libraries(scm PUBLIC aerie_includes Threads::Threads)

# pool_tool executable is defined in the parent src/CMakeLists.txt
# after libfs is assembled, so it can link against the single libfs target.
