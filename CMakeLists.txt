 cmake_minimum_required(VERSION 3.31.6)

 project(Chip-8 C)

find_package(raylib REQUIRED)
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wextra")
 set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out)
 add_executable(Chip-8 main.c lib/log.c
         lib/cpu.h
         lib/cpu.c)

 target_include_directories(Chip-8 INTERFACE lib)
 target_link_libraries(Chip-8 PRIVATE raylib)
