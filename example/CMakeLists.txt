file(GLOB SOURCES "*/*.cpp")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/example)

add_library(libtrading STATIC ${SOURCES})
target_link_libraries(libtrading PRIVATE fmt::fmt-header-only  ${LIBUV_LIBRARIES} libexchange)
target_include_directories(libtrading PRIVATE ${LIBUV_INCLUDE_DIRS})

add_executable(trading_system_example main.cpp)
target_link_libraries(trading_system_example PRIVATE libtrading)