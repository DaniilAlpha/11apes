cmake_minimum_required(VERSION 3.5)

project(pdp11emu VERSION 1.0.0 LANGUAGES C)


set(SRCS_PATH "src/*.c")
set(INCLUDE_DIR "include/" "external/result/")

file(GLOB_RECURSE SRCS ${SRCS_PATH})


set(MAIN "main")

add_executable(${MAIN} ${SRCS})
target_include_directories(${MAIN} PRIVATE ${INCLUDE_DIR})

set(COMPILE_AND_BUILD_FlAGS
    -Og -g
    -std=gnu23
    -W -Wall -Wextra -Wformat -Wno-unused-parameter
    -fsanitize=address)
target_compile_options(${MAIN} PRIVATE ${COMPILE_AND_BUILD_FlAGS})
target_link_options(${MAIN} PRIVATE ${COMPILE_AND_BUILD_FlAGS})


