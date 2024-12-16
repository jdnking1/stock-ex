include(Testing)

add_executable(test "test.cpp" )

target_link_libraries(test PRIVATE src)

AddTests(test)