#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/poll.h>

#include "poll_worker.h"
#include "connection.h"
#include "logger.h"

#define MAX_EVENTS 2000

void start_poll(int listen_sock) {
    int connfd, sockfd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    struct pollfd clientfds[MAX_EVENTS];
    int maxi;
    int count_fds;

    //Add listening descriptor
    clientfds[0].fd = listen_sock;
    clientfds[0].events = POLLIN;


    for (int i = 1; i < MAX_EVENTS; i++) {
        clientfds[i].fd = -1;
        clientfds[i].events = 0;
    }

    maxi = 0;
    //Loop proc
//
//    int epoll_fd, count_fds;
//    struct epoll_event events[MAX_EVENTS];
    connection_t *connections = calloc(MAX_EVENTS, sizeof(connection_t));
    for (int i = 0; i < MAX_EVENTS; ++i) {
        connections[i].fd = -1;

    }
//
//    if ((epoll_fd = epoll_create1(0)) == -1) {
//        exit(1);
//    }
//
//    struct epoll_event ev_listen_sock;
//    ev_listen_sock.events = EPOLLIN | EPOLLEXCLUSIVE;
//    ev_listen_sock.data.fd = listen_sock;
//
//    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev_listen_sock) == -1) {
//        exit(1);
//    }

    while (1) {
        if ((count_fds = poll(clientfds, MAX_EVENTS, -1)) < 0) {
            LOG(FATAL, "error epoll_wait: %s", strerror(errno));
            exit(1);
        }

        if (clientfds[0].events & POLLIN) {
            // ассептим соединение
            int num_connection = accept_connection_poll(connections, listen_sock, MAX_EVENTS);
            if (num_connection < 0) {
                LOG(WARNING, "cannot accept connection: %s", strerror(errno));
                continue;
            }
            num_connection++;
            clientfds[num_connection].fd = connections[num_connection].fd;
            clientfds[num_connection].events = POLLIN;

        }
        for (int j = 1; j < MAX_EVENTS; ++j) {
            if (clientfds[j].events & POLLIN) { // есть данные на чтение
                connection_t *c = connections + j - 1;
                connection_serve_processing_t serve_result = connection_serve(c);
                if (serve_result == SERVE_OK) {
                    clientfds[j].events = 0;
                    clear_connection(c);
                } else if (serve_result == SERVE_STOP) {
                    clientfds[j].events = 0;
                    clear_connection(c);
                } else {
                    switch (c->status) {
                        case SEND_HEADER:
                        case SEND_BODY: {
                            clientfds[j].events = POLLOUT;
                        }
                    }
                }
            } else if (clientfds[j].events & POLLOUT) {
                connection_t *c = connections + j - 1;
                connection_serve_processing_t serve_result = connection_serve(c);

                if (serve_result == SERVE_OK) { // если успешно обработали - закрываем коннекшн
                    LOG(INFO, "connection successfully processed, fd %d", c->fd);
                    clear_connection(c);
                    clientfds[j].events = 0;
                } else if (serve_result == SERVE_STOP) {
                    LOG(INFO, "connection stop processed, fd %d", c->fd);
                    clear_connection(c);
                    clientfds[j].events = 0;
                }
            } else {
                clientfds[j].events = 0;
                //clear_connection(connections + j);
            }
        }

    }

    free(connections);
}