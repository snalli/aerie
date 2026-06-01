# ---------------------------------------------------------------------------
# Unit tests — Google Test
# ---------------------------------------------------------------------------
find_package(GTest REQUIRED)

set(unittest_incdirs
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AERIE_LIBFS_SOURCE_DIR}/src
    ${AERIE_LIBFS_SOURCE_DIR}
)

set(unittest_libs
    GTest::GTest
    pxfsc mfsclt osdclt bcsclt scm common
    Threads::Threads ${RT_LIB}
)

# All test sources
set(unittest_srcs
    main.cc
    client/file.test.cc
    client/namespace.test.cc
    common/bitset.test.cc
    common/interval_tree.test.cc
    osd/containers/byte.test.cc
    osd/containers/hashtable.test.cc
    osd/containers/name.test.cc
    osd/containers/openhashtable.test.cc
    osd/containers/radixtree.test.cc
    osd/gtque.test.cc
    osd/lock_protocol.test.cc
    scm/memcpy.test.cc
    scm/pheap.test.cc
    scm/pool.test.cc
    scm/pregion.test.cc
)

add_executable(aerie_unittests ${unittest_srcs})
target_include_directories(aerie_unittests PRIVATE ${unittest_incdirs})
target_link_libraries(aerie_unittests PRIVATE ${unittest_libs})
# Test mocks have many stub methods with intentionally unused parameters
target_compile_options(aerie_unittests PRIVATE -Wno-unused-parameter -Wno-unused-variable)
