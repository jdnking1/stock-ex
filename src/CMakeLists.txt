add_library(src "main.cpp" "matching_engine.cpp" "order_book.cpp" "order.cpp" "common.cpp" "engine_utils.cpp")
 
add_subdirectory(utils)

target_include_directories(src PUBLIC .)