
add_library(minico SHARED co.c vth_pool.c)
target_include_directories(minico PUBLIC .)

add_library(minico_static STATIC co.c vth_pool.c)
target_include_directories(minico PUBLIC .)