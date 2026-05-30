# ---------------------------------------------------------------------------
# ubench – micro-benchmark programs
# ---------------------------------------------------------------------------

set(bench_incdirs
    ${CMAKE_SOURCE_DIR}/bench
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)

# Per-filesystem client dependency sets
set(osd_libs   mfsclt fsc bcsclt osdclt scm common Threads::Threads ${RT_LIB})
set(pxfs_libs  mfsclt fsc bcsclt osdclt scm common Threads::Threads ${RT_LIB})
set(cfs_libs   cfsc       bcsclt osdclt scm common Threads::Threads ${RT_LIB})
set(rxfs_libs  rxfsc      bcsclt osdclt scm common Threads::Threads ${RT_LIB})
set(kvfs_libs  kvfsc      bcsclt osdclt scm common Threads::Threads ${RT_LIB})

# FS benchmark common sources (shared across all FS flavours)
set(fs_common
    fs/open.cc fs/create.cc fs/unlink.cc fs/read.cc fs/fs.cc
    fs/fread.cc fs/seqread.cc fs/randread.cc fs/seqwrite.cc
    fs/randwrite.cc fs/delete.cc fs/append.cc fs/rename.cc
)

# OSD-level benchmark objects
set(osd_sources
    osd/create.cc osd/open.cc osd/lock.cc osd/hlock.cc osd/rpc.cc osd/osd.cc
)

# ---------------------------------------------------------------------------
# ubench_osd
# ---------------------------------------------------------------------------
add_executable(ubench_osd main.cc ${osd_sources})
target_include_directories(ubench_osd PRIVATE ${bench_incdirs})
target_link_libraries(ubench_osd PRIVATE ${osd_libs})

# ---------------------------------------------------------------------------
# ubench_vfs  (plain POSIX, no libfs)
# ---------------------------------------------------------------------------
add_executable(ubench_vfs main.cc ${fs_common} fs/vfs.cc)
target_include_directories(ubench_vfs PRIVATE ${bench_incdirs})
target_link_libraries(ubench_vfs PRIVATE Threads::Threads ${RT_LIB})

# ---------------------------------------------------------------------------
# ubench_pxfs
# ---------------------------------------------------------------------------
add_executable(ubench_pxfs main.cc ${fs_common} fs/pxfs.cc)
target_include_directories(ubench_pxfs PRIVATE ${bench_incdirs})
target_link_libraries(ubench_pxfs PRIVATE ${pxfs_libs})

# ---------------------------------------------------------------------------
# pxfs_api_test  — correctness unit tests for every libfs_* API
# ---------------------------------------------------------------------------
add_executable(pxfs_api_test pxfs_api_test.cc)
target_include_directories(pxfs_api_test PRIVATE ${bench_incdirs})
target_link_libraries(pxfs_api_test PRIVATE ${pxfs_libs})

# ---------------------------------------------------------------------------
# kvfs_api_test  — correctness unit tests for every kvfs_* API
# ---------------------------------------------------------------------------
add_executable(kvfs_api_test kvfs_api_test.cc)
target_include_directories(kvfs_api_test PRIVATE ${bench_incdirs})
target_link_libraries(kvfs_api_test PRIVATE ${kvfs_libs})

# ---------------------------------------------------------------------------
# ubench_cfs
# ---------------------------------------------------------------------------
add_executable(ubench_cfs main.cc ${fs_common} fs/cfs.cc)
target_include_directories(ubench_cfs PRIVATE ${bench_incdirs})
target_link_libraries(ubench_cfs PRIVATE ${cfs_libs})

# ---------------------------------------------------------------------------
# ubench_rxfs
# ---------------------------------------------------------------------------
add_executable(ubench_rxfs main.cc ${fs_common} fs/rxfs.cc)
target_include_directories(ubench_rxfs PRIVATE ${bench_incdirs})
target_link_libraries(ubench_rxfs PRIVATE ${rxfs_libs})

# ---------------------------------------------------------------------------
# Multi-client programs
# ---------------------------------------------------------------------------
add_executable(clt01 clt01.cc ${fs_common} fs/pxfs.cc)
target_include_directories(clt01 PRIVATE ${bench_incdirs})
target_link_libraries(clt01 PRIVATE ${pxfs_libs})

add_executable(clt02 clt02.cc ${fs_common} fs/pxfs.cc)
target_include_directories(clt02 PRIVATE ${bench_incdirs})
target_link_libraries(clt02 PRIVATE ${pxfs_libs})

add_executable(clt03 clt03.cc)
target_include_directories(clt03 PRIVATE ${bench_incdirs})
target_link_libraries(clt03 PRIVATE Threads::Threads ${RT_LIB})

add_executable(bench_read read.cc ${fs_common} fs/pxfs.cc)
target_include_directories(bench_read PRIVATE ${bench_incdirs})
target_link_libraries(bench_read PRIVATE ${pxfs_libs})
