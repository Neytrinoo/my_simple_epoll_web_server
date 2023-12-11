#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "epool_worker.h"
#include "connection.h"
#include "logger.h"

#define MAX_EVENTS 128

void start(int listen_sock) {
    int epoll_fd, count_fds;
    struct epoll_event events[MAX_EVENTS];

    connection_t *connections = calloc(MAX_EVENTS, sizeof(connection_t));
    for (int i = 0; i < MAX_EVENTS; ++i) {
        connections[i].fd = -1;
    }

    if ((epoll_fd = epoll_create1(0)) == -1) {
        exit(1);
    }

    struct epoll_event ev_listen_sock;
    ev_listen_sock.events = EPOLLIN | EPOLLEXCLUSIVE;
    ev_listen_sock.data.fd = listen_sock;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev_listen_sock) == -1) {
        exit(1);
    }

    while (1) {
        if ((count_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1)) < 0) {
            LOG(FATAL, "error epoll_wait: %s", strerror(errno));
            exit(1);
        }

        for (int i = 0; i < count_fds; ++i) {
            if (events[i].data.fd == listen_sock) {
                // ассептим соединение
                connection_t *connection = accept_connection(connections, listen_sock, MAX_EVENTS);
                if (!connection) {
                    LOG(WARNING, "cannot accept connection: %s", strerror(errno));
                    continue;
                }
                LOG(INFO, "accept1 connection, fd %d", connection->fd);

                // добавляем его в массив epool_event'ов
                struct epoll_event ev_conn;
                ev_conn.events = EPOLLIN | EPOLLET;
                ev_conn.data.fd = connection->fd;
                ev_conn.data.ptr = connection;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection->fd, &ev_conn) == -1) {
                    LOG(FATAL, "cannot add client event to epoll events: %s", strerror(errno));
                    exit(1);
                }
                LOG(INFO, "accept2 connection, fd %d", connection->fd);

            } else if (events[i].events & EPOLLIN) { // есть данные на чтение
                connection_t *c = events[i].data.ptr;
                connection_serve_processing_t serve_result = connection_serve(c);
                if (serve_result == SERVE_OK) {
                    LOG(INFO, "connection successfully processed, fd %d", c->fd);
                    clear_connection(c);
                } else if (serve_result == SERVE_STOP) {
                    LOG(INFO, "connection stop processed, fd %d", c->fd);
                    clear_connection(c);
                } else {
                    switch (c->status) {
                        case SEND_HEADER:
                        case SEND_BODY: {
                            struct epoll_event ev_conn;

                            ev_conn.events = EPOLLOUT | EPOLLET;
                            ev_conn.data.ptr = (void *) c;

                            if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, c->fd, &ev_conn) < 0) {
                                LOG(FATAL, "cannot mod client event in epoll events: %s", strerror(errno));
                                exit(1);
                            }
                        }
                    }
                }
            } else if (events[i].events & EPOLLOUT) { // есть возможность записать ответ
                connection_t *c = events[i].data.ptr;
                connection_serve_processing_t serve_result = connection_serve(c);

                if (serve_result == SERVE_OK) { // если успешно обработали - закрываем коннекшн
                    LOG(INFO, "connection successfully processed, fd %d", c->fd);
                    struct epoll_event e;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c->fd, &e);
                    clear_connection(c);
                } else if (serve_result == SERVE_STOP) {
                    LOG(INFO, "connection stop processed, fd %d", c->fd);
                    struct epoll_event e;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c->fd, &e);
                    clear_connection(c);
                }
            } else {
                clear_connection(events[i].data.ptr);
            }
        }
    }

    free(connections);
}
