file(GLOB SOURCES "*/*.cpp")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(libexchange STATIC ${SOURCES})

target_link_libraries(libexchange PRIVATE fmt::fmt-header-only)

add_executable(order_book "engine/order_book_example.cpp")
target_link_libraries(order_book PRIVATE libexchange)