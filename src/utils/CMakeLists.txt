file(GLOB HEADER_FILES "*.hpp")

add_library(libutils INTERFACE)

target_sources(libutils INTERFACE ${HEADER_FILES})

target_include_directories(libutils INTERFACE .)

if(UNIX)
    target_link_libraries(libutils INTERFACE pthread)
endif()

add_executable(thread_example  "thread_example.cpp")

add_executable(memory_pool_example "memory_pool_example.cpp")

add_executable(lock_free_queue_example "lock_free_queue_example.cpp")

add_executable(logger_example "logger_example.cpp")

add_executable(poco_example "poco_example.cpp")

find_package(Poco CONFIG REQUIRED Foundation Net)

target_link_libraries(poco_example PRIVATE Poco::Foundation Poco::Net)

