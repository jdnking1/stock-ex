file(GLOB SOURCES "*.cpp")

include_directories(${PROJECT_SOURCE_DIR})

add_library(libutils STATIC ${SOURCES})

if(UNIX)
    target_link_libraries(libutils PRIVATE pthread)
endif()