file(GLOB SOURCES "*/*.cpp")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(libexchange STATIC ${SOURCES})

target_link_libraries(libexchange PRIVATE fmt::fmt-header-only  ${LIBUV_LIBRARIES})
target_include_directories(libexchange PRIVATE ${LIBUV_INCLUDE_DIRS})

