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

#include "http.h"
#include "file_reader.h"

#include "epool_worker.h"
#include "logger.h"

char *ROOT = "/home/dmitriy/vuzic/network/course_work/static";

#define MAX_CONN 4096
#define COUNT_WORKER_PROCESS 1
#define PORT 9090

int main() {
    int listen_sock;
    pid_t worker_pids[COUNT_WORKER_PROCESS];
    set_log_file(stdout);

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

    for (int i = 0; i < COUNT_WORKER_PROCESS; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            LOG(FATAL, "cannot fork, %s", strerror(errno));
            return -1;
        }
        if (pid != 0) {
            LOG(INFO, "create worker process, pid %d", pid);
            worker_pids[i] = pid;
        } else {
            start(listen_sock);
            break;
        }
    }

    if (getpid() == main_pid) {
        LOG(INFO, "server start listening, port: %d", PORT);
        start(listen_sock);
        for (int i = 0; i < COUNT_WORKER_PROCESS; ++i) {
            waitpid(worker_pids[i], NULL, 0);
        }
    }

    LOG(INFO, "server stopped");

    return 0;
}
