cmake_minimum_required(VERSION 3.10.0)
project(Limit-Order-Book-Simulator VERSION 0.1.0 LANGUAGES C CXX)

add_executable(Limit-Order-Book-Simulator src/main.cpp Limit_Order_Book/Limit.cpp Limit_Order_Book/Book.cpp Limit_Order_Book/Book.hpp Limit_Order_Book/Limit.hpp Limit_Order_Book/Order.cpp Limit_Order_Book/Order.hpp)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
message("CMake is located at: ${CMAKE_COMMAND}")
include(CPack)
