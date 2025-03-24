cmake_minimum_required(VERSION 3.5)

project(pdp11emu-test VERSION 0.1.0 LANGUAGES C)


set(SRC_PATHS "test.c" "src/*.c")
set(INCLUDE_DIRS "include/" "external/miunte/")

file(GLOB_RECURSE SRCS ${SRC_PATHS})


set(TEST "test")

add_executable(${TEST} ${SRCS})
target_include_directories(${TEST} PRIVATE ${INCLUDE_DIRS})

set(COMPILE_AND_BUILD_FlAGS
    -Og -g
    -std=gnu23
    -W -Wall -Wextra -Wformat -Winline -Wno-unused-parameter
    -fsanitize=address)
target_compile_options(${TEST} PRIVATE ${COMPILE_AND_BUILD_FlAGS})
target_link_options(${TEST} PRIVATE ${COMPILE_AND_BUILD_FlAGS})


add_subdirectory("../lib/" "${CMAKE_BINARY_DIR}/lib/")
target_link_libraries(${TEST} lib)


