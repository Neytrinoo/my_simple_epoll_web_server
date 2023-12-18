#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "file_reader.h"
#include "logger.h"

file_reader_t *new_file_reader(file_type_t *file_type) {
    file_reader_t *file_reader = malloc(sizeof(file_reader_t));
    if (!file_reader) {
        return NULL;
    }

    file_reader->fd = open(file_type->file_path->data, O_RDONLY);
    if (file_reader->fd == -1) {
        free(file_reader);
        return NULL;
    }

    file_reader->bytes_to_send = 0;
    file_reader->has_bytes_to_read = true;
    file_reader->now_send_byte = 0;

    return file_reader;
}

void free_file_reader(file_reader_t **fr) {
    close((*fr)->fd);

    free(*fr);
    *fr = NULL;
}


ssize_t read_next(file_reader_t *fr) {
    return read(fr->fd, fr->buf, FILE_BUF_SIZE);
}

send_file_status_t send_next(file_reader_t *fr, int sock_fd) {
    while (1) {
        if (!(fr->bytes_to_send - fr->now_send_byte)) { // когда все отправили - читаем заново
            fr->bytes_to_send = read_next(fr);
            if (!fr->bytes_to_send) { // если все считали - возвращаем OK
                return SEND_FILE_OK;
            }
            fr->now_send_byte = 0;
        }

        ssize_t wn = write(sock_fd, fr->buf + fr->now_send_byte, fr->bytes_to_send - fr->now_send_byte);
        if (wn <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) { // ошибки нет, но мы не успели записать все - продолжим когда будет возможность
                return SEND_FILE_CONTINUE;
            }
            return SEND_FILE_ERROR;
        } else {
            fr->now_send_byte += wn;
        }
    }
}