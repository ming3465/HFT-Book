cmake_minimum_required(VERSION 3.10.0)
project(Limit-Order-Book-Simulator VERSION 0.1.0 LANGUAGES C CXX)

add_executable(Limit-Order-Book-Simulator main.cpp)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
