file(GLOB SOURCES "*.cpp")

add_library(libutils STATIC ${SOURCES})

target_include_directories(libutils PUBLIC .)
