cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(SkipList LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if (CYGWIN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--enable-auto-import")
endif()

add_compile_options(-g)
# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

set(SRCS main/main.cpp)
aux_source_directory(src/ SRCS)
add_executable(skip_list ${SRCS})

# if(MSVC)
#     target_compile_options(skip_list PRIVATE /W4)
# else()
#     target_compile_options(skip_list PRIVATE -Wall -Wextra -pedantic)
# endif()

include_directories(src/)
# target_include_directories(skip_list PUBLIC include)



