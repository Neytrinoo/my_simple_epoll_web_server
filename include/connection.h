#ifndef COURSE_WORK_CONNECTION_H
#define COURSE_WORK_CONNECTION_H

#include <netinet/in.h>
#include "http.h"

typedef enum {
    RECV_HEADER, // считываем хедер
    SEND_HEADER, // отправляем хедер ответа
    SEND_BODY // отправляем тело ответа
} connection_status_t;

typedef enum {
    SERVE_CONTINUE, // обработка не закончена
    SERVE_CHANGE_STATUS_TO_WRITE, // выставляется, когда все данные приняли и начинаем записывать
    SERVE_OK, // обработка закончена
    SERVE_STOP // ошибка, завершаем обработку
} connection_serve_processing_t;

typedef struct {
    int fd; // сокет соединения
    http_req_t *req; // запрос
    http_resp_t *resp; // ответ
    connection_status_t status; // статус соединения
    struct sockaddr_in client_addr;
} connection_t;

int accept_connection_poll(connection_t *connections, int listen_sock, size_t count_connections);

connection_t *accept_connection(connection_t *connections, int listen_sock, size_t count_connections);

int set_nonblocking(int sockfd);

// функция заполнения соединения
void fill_connection(connection_t *connection, int fd);

// функция очистки соединения
void clear_connection(connection_t *connection);

// функция обработки соединения
connection_serve_processing_t connection_serve(connection_t *connection);

#endif //COURSE_WORK_CONNECTION_H
