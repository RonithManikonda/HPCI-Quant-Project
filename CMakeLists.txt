cmake_minimum_required(VERSION 3.20)
project(orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(ob
  src/symbol_book.cc
  src/order_book.cc
)
target_include_directories(ob PUBLIC include)
target_compile_options(ob PRIVATE -Wall -Wextra -Wpedantic)

add_library(ob_ingest
  src/ingest/soupbin.cc
  src/ingest/itch.cc
)
target_include_directories(ob_ingest PUBLIC include)
target_compile_options(ob_ingest PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(ob_ingest PUBLIC ob)

add_executable(ob_demo src/main.cc)
target_link_libraries(ob_demo PRIVATE ob)

add_executable(ob_itch_ingest src/itch_ingest_main.cc)
target_link_libraries(ob_itch_ingest PRIVATE ob_ingest)


add_executable(ob_tests tests/test_order_book.cc)
target_link_libraries(ob_tests PRIVATE ob)
