#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <sys/resource.h>

#include "http.h"
#include "file_reader.h"

#include "epoll_worker.h"
#include "logger.h"
#include "poll_worker.h"

char *ROOT = "/home/dmitriy/vuzic/network/course_work/static";

#define MAX_CONN 4096

int COUNT_WORKER_PROCESS = 1;
int PORT = 9090;
bool IS_POLL = false;

void set_port(char *argv[]) {
    PORT = atoi(argv[1]);;
}

void set_count_workers(char *argv[]) {
    COUNT_WORKER_PROCESS = atoi(argv[2]);
    if (COUNT_WORKER_PROCESS > 100) {
        exit(1);
    }
}

void set_root_dir(char *argv[]) {
    ROOT = argv[3];
}

void set_is_poll(char *argv[]) {
    if (!strcmp(argv[4], "true")) {
        IS_POLL = true;
    } else {
        IS_POLL = false;
    }
}

void set_params(char *argv[]) {
    set_port(argv);
    set_count_workers(argv);
    set_root_dir(argv);
    set_is_poll(argv);
}

// main <port> <count_workers> <root_dir> <is_poll>
int main(int argc, char *argv[]) {
    int listen_sock;
    pid_t worker_pids[COUNT_WORKER_PROCESS];
    set_log_file(stdout);

    if (argc != 5) {
        LOG(FATAL, "error args");
        exit(1);
    }

    set_params(argv);

    struct rlimit rlim;
    rlim.rlim_cur = rlim.rlim_max = COUNT_WORKER_PROCESS * (MAX_CONN + 1);
    if (setrlimit(RLIMIT_NOFILE, &rlim) < 0) {
        if (errno == EPERM) {
            LOG(FATAL, "You need to run as root or have "
                "CAP_SYS_RESOURCE set, or are asking for more "
                "file descriptors than the system can offer");
            exit(1);
        } else {
            LOG(FATAL, "setrlimit error: %s", strerror(errno));
        }
    }

    struct sockaddr_in server_addr;
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        LOG(FATAL, "cannot init listen sock, %s", strerror(errno));
        exit(1);
    }

    int enable = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
        LOG(FATAL, "cannot set reuse for listen sock, %s", strerror(errno));
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        LOG(FATAL, "cannot bind listen sock, %s", strerror(errno));
        exit(1);
    }
    // set nonblocking
    if (fcntl(listen_sock, F_SETFD, fcntl(listen_sock, F_GETFD, 0) | O_NONBLOCK) == -1) {
        LOG(FATAL, "cannot set nonblocking listen sock, %s", strerror(errno));
        exit(1);
    }

    if (listen(listen_sock, MAX_CONN) == -1) {
        exit(1);
    }
    //start(listen_sock);
    pid_t main_pid = getpid();
    signal(SIGPIPE, SIG_IGN);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGCHLD);

    for (int i = 0; i < COUNT_WORKER_PROCESS-1; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            LOG(FATAL, "cannot fork, %s", strerror(errno));
            return -1;
        }
        if (pid != 0) {
            worker_pids[i] = pid;
        } else {
            if (IS_POLL) {
                start_poll(listen_sock);
            } else {
                start(listen_sock);
            }
            break;
        }
    }

    if (getpid() == main_pid) {
        LOG(INFO, "server start listening, port: %d", PORT);
        if (IS_POLL) {
            LOG(DEBUG, "start poll");
            start_poll(listen_sock);
        } else {
            start(listen_sock);
        }
        for (int i = 0; i < COUNT_WORKER_PROCESS-1; ++i) {
            waitpid(worker_pids[i], NULL, 0);
        }
    }

    LOG(INFO, "server stopped");

    return 0;
}
