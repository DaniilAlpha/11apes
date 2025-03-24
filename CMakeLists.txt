cmake_minimum_required(VERSION 3.5)

project(pdp11emu-main VERSION 0.1.0 LANGUAGES C)


set(SRCS "main.c")
set(INCLUDE_DIR "include/")


set(MAIN "main")

add_executable(${MAIN} ${SRCS})
target_include_directories(${MAIN} PRIVATE ${INCLUDE_DIR})

set(COMPILE_AND_BUILD_FlAGS
    -Og -g
    -std=gnu23
    -W -Wall -Wextra -Wformat -Winline -Wno-unused-parameter
    -fsanitize=address)
target_compile_options(${MAIN} PRIVATE ${COMPILE_AND_BUILD_FlAGS})
target_link_options(${MAIN} PRIVATE ${COMPILE_AND_BUILD_FlAGS})


add_subdirectory("./lib/" "${CMAKE_BINARY_DIR}/lib/")
target_link_libraries(${MAIN} lib)
