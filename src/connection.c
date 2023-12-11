#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include "connection.h"
#include "logger.h"

int set_nonblocking(int sockfd) {
    if (fcntl(sockfd, F_SETFD, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1) {
        return -1;
    }

    return 0;
}

void fill_connection(connection_t *connection, int fd) {
    connection->req = new_http_req();
    connection->status = RECV_HEADER;
}

void clear_connection(connection_t *connection) {
    if (connection->resp != NULL) {
        free_http_resp(&connection->resp); // req очистится внутри
    } else {
        free_http_req_t(&connection->req);
    }

    shutdown(connection->fd, SHUT_RDWR);
    close(connection->fd);
    memset(connection, 0, sizeof(connection_t));
    connection->fd = -1;
}

connection_t *accept_connection(connection_t *connections, int listen_sock, size_t count_connections) {
    connection_t *c = NULL;
    size_t i;

    // ищем первый свободный коннекшн
    for (i = 0; i < count_connections; i++) {
        if (connections[i].fd == -1) {
            c = &connections[i];
            c->fd = 1;
            break;
        }
    }

    if ((c->fd = accept(listen_sock, (struct sockaddr *) &c->client_addr, &(socklen_t) {sizeof(c->client_addr)})) < 0) {
        return NULL;
    }

    if (set_nonblocking(c->fd)) {
        return NULL;
    }

    fill_connection(c, c->fd);

    return c;
}

connection_serve_processing_t connection_serve(connection_t *connection) {
    switch (connection->status) {
        case RECV_HEADER: { // считываем хедер
            connection->req->recv_header_result = read_raw_header(connection->fd, connection->req);
            if (connection->req->recv_header_result != RW_CONTINUE) { // если уже считали
                switch (connection->req->recv_header_result) {
                    case RW_REQUEST_TIMEOUT:
                    case RW_BAD_REQUEST: // в случае ошибки просто выходим
                        return SERVE_STOP;
                    case RW_OK: {
                        // если считали успешно - парсим и выставляем в req->parse_result результат парсинга
                        connection->req->parse_result = parse_http_header(connection->req);
                    }
                }
                // создаем структуру ответа и переходим к статусу отправки хедера
                connection->resp = new_http_resp(connection->req);
                LOG(INFO, "REQUEST: %s %s", get_method_string(connection->req->method), connection->req->file_type->raw_file_path->data);
                connection->status = SEND_HEADER;

                // возвращаем, что начали писать, чтобы сменить флаг epool у сокета клиента
                /* fallthrough */
            }
        }
        case SEND_HEADER: { // статус отправки хедера
            rw_result_t write_result = send_http_resp_header(connection->fd, connection->resp);

            switch (write_result) {
                case RW_OK: // если отправили успешно
                    LOG(INFO, "RESPONSE: %s %s %d", get_method_string(connection->req->method), connection->req->file_type->raw_file_path->data, connection->resp->status_code);
                    if (connection->req->method == GET && connection->resp->status_code == S_OK) { // и это был GET метод и все ок
                        connection->status = SEND_BODY; // то переходим к отправке тела
                        connection->resp->file_reader = new_file_reader(connection->req->file_type); // открываем файл
                    } else { // если был любой другой метод - обработка окончена
                        return SERVE_OK;
                    }
                    break;
                case RW_CONTINUE:
                    break;
                default:
                    return SERVE_OK;
            }
        }
        case SEND_BODY: { // отправка тела
            send_file_status_t send_result = send_next(connection->resp->file_reader, connection->fd);
            if (send_result == SEND_FILE_OK) { // если все отправили - закрываем файл
                free_file_reader(&connection->resp->file_reader);
                return SERVE_OK;
            } else if (send_result == SEND_FILE_ERROR) {
                free_file_reader(&connection->resp->file_reader);
                return SERVE_STOP;
            }
            break;
        }
    }
    // в любом другом случае продолжаем обработку
    return SERVE_CONTINUE;
}