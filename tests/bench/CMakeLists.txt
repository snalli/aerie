add_subdirectory(ubench)

# sharing bench (VFS-only, no libfs dependency)
add_executable(sharing_vfs
    sharing/main.cc
    sharing/create-vfs.cc
    sharing/open-vfs.cc
    sharing/barrier.cc
)
target_include_directories(sharing_vfs PRIVATE
    ${CMAKE_SOURCE_DIR}/../tests/bench
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/../tests
)
target_link_libraries(sharing_vfs PRIVATE Threads::Threads ${RT_LIB})
