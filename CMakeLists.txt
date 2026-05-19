cmake_minimum_required(VERSION 3.10.0)
project(Limit-Order-Book-Simulator VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(lob_core
    Limit_Order_Book/Book.cpp
    Limit_Order_Book/Limit.cpp
    Limit_Order_Book/Order.cpp
    Generate_Book/GenerateBook.cpp
    Fetch_Lobster_data/LobsterReader.cpp)
target_include_directories(lob_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Limit_Order_Book
    ${CMAKE_CURRENT_SOURCE_DIR}/Generate_Book
    ${CMAKE_CURRENT_SOURCE_DIR}/Fetch_Lobster_data)
target_link_libraries(lob_core PUBLIC Threads::Threads)

add_executable(lob_smoke src/main.cpp)
target_link_libraries(lob_smoke PRIVATE lob_core)

add_executable(lob_bench bench/bench_main.cpp)
target_link_libraries(lob_bench PRIVATE lob_core)
if (WIN32)
    target_link_libraries(lob_bench PRIVATE psapi)
endif()

add_executable(lob_replay bench/replay_main.cpp)
target_link_libraries(lob_replay PRIVATE lob_core)

add_executable(lob_sim bench/sim_main.cpp)
target_link_libraries(lob_sim PRIVATE lob_core)

# Keep the original simulator target name as an alias of lob_smoke.
add_executable(Limit-Order-Book-Simulator src/main.cpp)
target_link_libraries(Limit-Order-Book-Simulator PRIVATE lob_core)

include(CTest)
enable_testing()
add_test(NAME smoke COMMAND lob_smoke)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
