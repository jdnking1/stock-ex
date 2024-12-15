file(GLOB HEADER_FILES "*.hpp")

add_library(libutils INTERFACE)

target_sources(libutils INTERFACE ${HEADER_FILES})

target_include_directories(libutils INTERFACE .)

if(UNIX)
    target_link_libraries(libutils INTERFACE pthread)
endif()

add_executable(thread_example  "thread_example.cpp")