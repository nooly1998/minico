

add_executable(co_test co_test.c)
target_include_directories(co_test PRIVATE ../src)
target_link_libraries(co_test PRIVATE minico)
add_test(NAME co_test COMMAND co_test)


add_executable(pool_test pool_test.c)
target_include_directories(pool_test PRIVATE ../src)
target_link_libraries(pool_test PRIVATE minico)
add_test(NAME pool_test COMMAND pool_test)