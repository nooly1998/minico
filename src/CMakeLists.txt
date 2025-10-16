
find_package(Threads REQUIRED)

add_library(minico SHARED co.c vth_pool.c)
target_include_directories(minico PUBLIC .)
target_link_libraries(minico PRIVATE Threads::Threads)

add_library(minico_static STATIC co.c vth_pool.c)
target_include_directories(minico PUBLIC .)
target_link_libraries(minico PRIVATE Threads::Threads)
