cmake_minimum_required(VERSION 3.10)

project(pdp11emu VERSION 0.1.0 LANGUAGES C)


set(SRCS_PATH "src/*.c" "external/vec/src/*.c")
set(INCLUDE_DIRS "include/"
    "external/result/" "external/woodi/" "external/vec/include/")

file(GLOB_RECURSE SRCS ${SRCS_PATH})


set(LIB "lib")

add_library(${LIB} STATIC ${SRCS})
target_include_directories(${LIB} PUBLIC ${INCLUDE_DIRS})


set(COMPILE_AND_BUILD_FlAGS
    -std=gnu2x
    $<$<CONFIG:Debug>: -O0 -g > $<$<CONFIG:Release>: -O3 >
    -W -Wall -Wextra -Wformat $<$<CONFIG:Release>: -Winline >
    -fsanitize=address)
target_compile_options(${LIB} PRIVATE ${COMPILE_AND_BUILD_FlAGS})
target_link_options(${LIB} PRIVATE ${COMPILE_AND_BUILD_FlAGS})


