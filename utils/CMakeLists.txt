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

add_executable(poco_examplee "poco_example.cpp")

find_package(Poco CONFIG REQUIRED Foundation Net)

target_include_directories(libutils PRIVATE
    $ENV{Poco_FOUNDATION_INCLUDE}
   $ENV{Poco_NET_INCLUDE}
)

target_link_libraries(poco_examplee PRIVATE Poco::Foundation Poco::Net)

