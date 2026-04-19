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
aux_source_directory(src/ SRCS)
add_executable(skip_list ${SRCS} main/main.cpp)

find_package(GTest REQUIRED)

option(WITH_TEST "Build unit tests" OFF)
if (WITH_TEST)
    aux_source_directory(test SRCS)
    add_executable(skip_list_test ${SRCS})
    target_link_libraries(skip_list_test GTest::gtest GTest::gtest_main)
endif()

if (WITH_ASAN)
    MESSAGE("enable santitiser")
    target_link_libraries(skip_list asan)
    target_link_libraries(skip_list_test asan)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

include_directories(src/)



