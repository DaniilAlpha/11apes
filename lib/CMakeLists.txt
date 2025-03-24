cmake_minimum_required(VERSION 3.5)

project(pdp11emu VERSION 0.1.0 LANGUAGES C)


set(SRCS_PATH "src/*.c")
set(INCLUDE_DIR "include/" "external/result/")

file(GLOB_RECURSE SRCS ${SRCS_PATH})


set(LIB "lib")

add_library(${LIB} STATIC ${SRCS})
target_include_directories(${LIB} PUBLIC ${INCLUDE_DIR})

set(COMPILE_AND_BUILD_FlAGS
    -Og -g
    -std=gnu23
    -W -Wall -Wextra -Wformat -Winline -Wno-unused-parameter
    -fsanitize=address)
target_compile_options(${LIB} PRIVATE ${COMPILE_AND_BUILD_FlAGS})
target_link_options(${LIB} PRIVATE ${COMPILE_AND_BUILD_FlAGS})


