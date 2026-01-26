
find_package(Threads REQUIRED)

set(MINICO_SOURCES co.c vth_pool.c)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
  list(APPEND MINICO_SOURCES ctx_switch_x86_64.S)
endif()

add_library(minico SHARED ${MINICO_SOURCES})
target_include_directories(minico PUBLIC .)
target_link_libraries(minico PRIVATE Threads::Threads)

add_library(minico_static STATIC ${MINICO_SOURCES})
target_include_directories(minico PUBLIC .)
target_link_libraries(minico PRIVATE Threads::Threads)
