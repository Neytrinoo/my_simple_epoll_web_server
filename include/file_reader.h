#ifndef COURSE_WORK_FILE_READER_H
#define COURSE_WORK_FILE_READER_H

#include <stdio.h>
#include <stdbool.h>

#include "file.h"

#define FILE_BUF_SIZE 4096 // 4KB, размер страницы

typedef enum {
    SEND_FILE_CONTINUE,
    SEND_FILE_OK,
    SEND_FILE_ERROR
} send_file_status_t;

typedef struct {
    int fd;
    file_type_t *file_type;
    ssize_t bytes_to_send;
    ssize_t now_send_byte;
    bool has_bytes_to_read;
    char buf[4096];
} file_reader_t;

file_reader_t *new_file_reader(file_type_t *file_type);

void free_file_reader(file_reader_t **fr);

// считывает данные в buf и возвращает кол-во считанных символов (0 = файл полностью считан)
ssize_t read_next(file_reader_t *fr);

// отправляет следующую пачку байт в сокет
send_file_status_t send_next(file_reader_t *fr, int sock_fd);


#endif //COURSE_WORK_FILE_READER_H
