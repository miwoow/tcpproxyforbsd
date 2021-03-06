#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef __linux__
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define MAX_EVENT_COUNT 5000
#define MAX_RECV_BUFF 65535

struct client {
    int fd;
    struct in_addr client_addr;
    int port;
    int upstream_fd;
    struct in_addr upstream_addr;
    int upstream_port;  // upstream port in host byte order.
};

// 因为Linux无法保存两个event参数，所以这里包裹一层来解决这个问题
struct ev_data {
    int ev_fd;
    struct client *cli;
};


int server_start(int *sk, char *ip, int port);
int regist_sock(int kq, int fd, struct client *cli);
int clear_client(int kq, struct client *cli);


#endif
