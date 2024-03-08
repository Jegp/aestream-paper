cmake_minimum_required(VERSION 3.20)
project(aestream-paper)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O3 -Wall -DNDEBUG -march=native ${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

set(GCC_COVERAGE_COMPILE_FLAGS "-fcoroutines")
set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)

set(default_build_type Release)
add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

include_directories("aestream/include")
# add_subdirectory("aestream")

# Include dirs
include_directories(${PROJECT_SOURCE_DIR}/include)

# AEDAT4 processing
add_executable(aestream-paper

    include/aer.hpp
    include/aedat.hpp
    include/result.hpp
    include/task.hpp
    include/noop.hpp
    include/threads.hpp
    include/threadpool.hpp
    include/generator.hpp
    include/threadpool_coro.hpp
    include/benchmark/base.hpp
    include/benchmark/coro_threads.hpp
    # include/benchmark/coro.hpp
    include/benchmark/threadless.hpp
    # include/benchmark/tp_coro.hpp
    include/benchmark/tp.hpp

    # src/blocking_queue.cpp
    # src/noop.cpp
    #src/benchmark.cpp
    src/benchmark/base.cpp
    src/benchmark/coro_threads.cpp
    # src/benchmark/coro.cpp
    src/benchmark/threadless.cpp
    # src/benchmark/tp_coro.cpp
    src/benchmark/tp.cpp
    # src/tp_main.cpp
    src/benchmark.cpp
)

# add_executable(noop src/noop.cpp src/noop.hpp)
# target_link_libraries(aestream-paper PUBLIC aestream_lib)
# add_executable(interrupt src/interrupt.cpp)
# add_executable(interrupt2 src/interrupt2.cpp)
# add_executable(interrupt3 src/interrupt3.cpp)
