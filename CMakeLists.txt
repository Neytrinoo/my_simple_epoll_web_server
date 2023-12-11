cmake_minimum_required(VERSION 3.25)
project(course_work C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

aux_source_directory(src SRC)

add_executable(course_work  ${SRC} src/main.c src/file.c include/file.h src/file_reader.c include/file_reader.h src/epool_worker.c include/epool_worker.h src/connection.c include/connection.h src/logger.c include/logger.h)
