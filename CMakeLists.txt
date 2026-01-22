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

# set(SRCS main/main.cpp)
aux_source_directory(src/ SRCS)
add_executable(skip_list ${SRCS} main/main.cpp)

option(WITH_TEST "Build unit tests" OFF)
if(WITH_TEST)
    add_executable(skip_list_test ${SRCS} test/main_test.cpp)
    target_link_libraries(skip_list_test gtest gtest_main)
endif()

option(WITH_ASAN "enable santitiser" OFF)
if (WITH_ASAN)
    # target_link_libraries(skip_list lsan)
    # target_link_libraries(skip_list asan)
    # target_link_libraries(skip_list_test lsan)
    # target_link_libraries(skip_list_test asan)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -static-libasan)
endif()

include_directories(src/)
# target_include_directories(skip_list PUBLIC include)



