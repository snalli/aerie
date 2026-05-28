add_library(common SHARED
    hash.c
    interval_tree.cc
)
target_link_libraries(common PUBLIC aerie_includes)
