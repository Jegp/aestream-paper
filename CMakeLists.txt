cmake_minimum_required(VERSION 3.20)
project(aestream-paper)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O3 -Wall -march=native ${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
set(GCC_COVERAGE_COMPILE_FLAGS "-fcoroutines")

set(default_build_type Debug)
add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

# include_directories("aestream/include")
# add_subdirectory("aestream")

# AEDAT4 processing
# add_executable(aestream-paper src/benchmark.cpp src/blocking_queue.cpp)
add_executable(noop src/noop.cpp src/noop.hpp)
# target_link_libraries(aestream-paper PUBLIC aestream_lib)