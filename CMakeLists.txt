cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(SkipList LANGUAGES CXX)

MESSAGE("Building ${PROJECT_NAME} in ${CMAKE_SOURCE_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-g)
add_compile_options(-Wall)
add_compile_options(-Wextra)
add_compile_options(-Werror)

option(WITH_ASAN "enable santitiser" OFF)
if (WITH_ASAN)
    MESSAGE("enable santitiser")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

aux_source_directory(src/ SRCS)
aux_source_directory(src/mempool MEMPOOL_SRCS)
# DPDK 适配器仅在链接了 DPDK 库时编译
list(REMOVE_ITEM MEMPOOL_SRCS "${CMAKE_SOURCE_DIR}/src/mempool/dpdk_mempool.cpp")
add_executable(skip_list ${SRCS} ${MEMPOOL_SRCS} main/main.cpp)

find_package(GTest REQUIRED)

option(WITH_TEST "Build unit tests" OFF)
if (WITH_TEST)
    set(TEST_SRCS
        test/test_crud.cpp
        test/test_mempool.cpp
        test/test_range.cpp
        test/test_benchmark.cpp
    )
    list(REMOVE_ITEM MEMPOOL_SRCS "${CMAKE_SOURCE_DIR}/src/mempool/dpdk_mempool.cpp")
    add_executable(skip_list_test ${TEST_SRCS} ${SRCS} ${MEMPOOL_SRCS})
    target_link_libraries(skip_list_test GTest::gtest GTest::gtest_main)
endif()

include_directories(src/)
