cmake_minimum_required(VERSION 3.10)

project(Filehandle C)

add_executable(Filehandle
    src/mystrtok.c
    src/strhndl.c
    src/frequency.c
)

target_include_directories(Filehandle PRIVATE include)
target_compile_options(Filehandle PRIVATE -g3)
