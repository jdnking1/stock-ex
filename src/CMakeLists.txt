file(GLOB SOURCES "*/*.cpp")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(libexchange STATIC ${SOURCES})