cmake_minimum_required(VERSION 3.25)
project(course_work C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

aux_source_directory(src SRC)

add_executable(course_work  ${SRC})
