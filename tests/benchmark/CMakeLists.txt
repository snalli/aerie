add_subdirectory(ubench)

# sharing bench (VFS-only, no libfs dependency)
add_executable(sharing_vfs
    sharing/main.cc
    sharing/create-vfs.cc
    sharing/open-vfs.cc
    sharing/barrier.cc
)
target_include_directories(sharing_vfs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AERIE_LIBFS_SOURCE_DIR}/src
    ${AERIE_LIBFS_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(sharing_vfs PRIVATE Threads::Threads ${RT_LIB})
