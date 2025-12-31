cmake_minimum_required(VERSION 3.10)

project(Filehandle)

add_executable(Filehandle
    src/main.c
    src/mystrtok.c
)

target_include_directories(Filehandle PRIVATE include)
