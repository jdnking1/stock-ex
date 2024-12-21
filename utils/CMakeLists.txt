file(GLOB SOURCES "*.cpp")

include_directories(${PROJECT_SOURCE_DIR})

add_library(libutils STATIC ${SOURCES})

if(UNIX)
    target_link_libraries(libutils PRIVATE pthread)
endif()

add_executable(thread_example  "thread_example.cpp")

add_executable(memory_pool_example "memory_pool_example.cpp")

add_executable(lock_free_queue_example "lock_free_queue_example.cpp")

add_executable(logger_example "logger_example.cpp")

add_executable(libuv_example "libuv_example.cpp")

target_include_directories(libuv_example PRIVATE ${LIBUV_INCLUDE_DIRS})
target_link_libraries(libuv_example PRIVATE ${LIBUV_LIBRARIES})




