cmake_minimum_required(VERSION 3.20.0)

project(kse VERSION 1.0.0 LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(NoInSourceBuilds)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(WIN32)
    add_definitions(-D_WIN32)
    add_definitions(-DNOMINMAX)
elseif(UNIX)
    add_definitions(-D__linux__)
endif()

set(COMPILER_SPECIFIC_FLAGS "")
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(COMPILER_SPECIFIC_FLAGS "-Wall -Wextra -Werror -Wpedantic")
    string(APPEND CMAKE_CXX_FLAGS " ${COMPILER_SPECIFIC_FLAGS}")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(COMPILER_SPECIFIC_FLAGS "/W4 /WX")
    string(APPEND CMAKE_CXX_FLAGS " ${COMPILER_SPECIFIC_FLAGS}")
endif()

string(APPEND CMAKE_CXX_FLAGS " ${COMPILER_SPECIFIC_FLAGS}")

list(APPEND LIBS libutils)
list(APPEND LIBS libexchange)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/deps/fmt/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(deps/fmt)
add_subdirectory(utils)
add_subdirectory(src)
add_subdirectory(test)

add_executable(kse src/main.cpp)
target_link_libraries(kse PUBLIC ${LIBS})