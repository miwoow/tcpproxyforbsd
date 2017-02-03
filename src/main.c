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

#include "server.h"
#include "balance.h"
#include "conf.h"

char buf_[MAX_RECV_BUFF];
int listen_socket = 0;
struct conf_s opt = {0};

int handle_accept(int kq, int conn_size)
{
    int i=0;
    int ret = 0;
    for(i=0; i< conn_size; i++) {
        int client = accept(listen_socket, NULL, NULL);
        if (client == -1) {
            printf("error in accept\n");
            continue;
        }
        ret = regist_sock(kq, client, NULL);
        if (ret != 0) {
            printf("regist client failed\n");
            return -1;
        }
    }
    return 0;
}

int handle_receive(int kq, int sock, int avail_bytes, void *udata)
{
    struct ev_data *edata = (struct ev_data *)udata;
    struct client *cli = edata->cli;
    int ret = 0;

    if (avail_bytes == 0) {
        avail_bytes = MAX_RECV_BUFF;
    }

    memset(buf_, 0, MAX_RECV_BUFF);
    int bytes = recv(sock, buf_, avail_bytes, 0);
    char *pos = NULL;
    int len = 0;
    if (sock == cli->fd && cli->upstream_fd != -1) {
        // data from client
        if (bytes <= 0) {
            printf("%s:%d: client close or recv failed.\n", inet_ntoa(cli->client_addr), ntohs(cli->port));
perror("ERROR");
            clear_client(kq, cli);
            free(edata);
            edata = NULL;
            cli = NULL;
            return -1;
        }
        //printf("%s:%d: %s\n", inet_ntoa(cli->client_addr), ntohs(cli->port), buf_);

        if (avail_bytes == MAX_RECV_BUFF) {
            avail_bytes = bytes;
        }


        if (cli->upstream_fd == 0) {
            ret = balance_sel_server(kq, cli);
            if (ret < 0) {
                printf("balance_sel_server failed\n");
                return -1;
            }
        }
        pos = &buf_[0];
        len = avail_bytes;
        while(1) {
            //printf("send to upstream\n");
            ret = send(cli->upstream_fd, pos, len, 0);
            if (ret == 0) {
                clear_client(kq, cli);
            free(edata);
            edata = NULL;
                cli = NULL;
                printf("upstream close socket\n");
                return -1;
            }
            if (ret < 0) {
                clear_client(kq, cli);
            free(edata);
            edata = NULL;
                cli = NULL;
                printf("send to upstream failed\n");
                return -1;
            }
            if (ret < len) {
                printf("conntinue to sendto upstream\n");
                len -= ret;
                pos += ret;
            } else {
                break;
            }
        }
    } else if (sock == cli->upstream_fd && cli->fd != -1) {
        // data from upstream
        if (bytes <= 0) {
            printf("%s:%d: upstream close or recv failed.\n", inet_ntoa(cli->upstream_addr), cli->upstream_port);
perror("ERROR");
            clear_client(kq, cli);
            free(edata);
            edata = NULL;
            cli = NULL;
            return -1;
        }
        //printf("%s:%d: %s\n", inet_ntoa(cli->client_addr), ntohs(cli->port), buf_);

        if (avail_bytes == MAX_RECV_BUFF) {
            avail_bytes = bytes;
        }

        pos = &buf_[0];
        len = avail_bytes;
        while(1) {
            //printf("send to client\n");
            ret = send(cli->fd, pos, len, 0);
            if (ret == 0) {
                printf("client close socket\n");
                clear_client(kq, cli);
            free(edata);
            edata = NULL;
                cli = NULL;
                return -1;
            }
            if (ret < 0) {
                printf("send to client failed\n");
                clear_client(kq, cli);
            free(edata);
            edata = NULL;
                cli = NULL;
                return -1;
            }
            if (ret < len) {
                printf("conntinue to sendto client\n");
                len -= ret;
                pos += ret;
            } else {
                break;
            }
        }
    }

    return 0;
}

#ifdef __linux__
int handle_epoll_event(int kq, void *udata)
{
    struct ev_data *edata = (struct ev_data *)udata;
    int sock = edata->ev_fd;
    int ret = 0;

    if (sock == listen_socket) {
        // handle accept;
        int client = accept(listen_socket, NULL, NULL);
        if (client == -1) {
            printf("error in accept\n");
            return -1;
        }
        ret = regist_sock(kq, client, NULL);
        if (ret != 0) {
            printf("regist client failed\n");
            return -1;
        }
    } else {
        handle_receive(kq, sock, 0, udata);
    }
    return 0;
}
#endif

#ifdef __linux__
void handle_event(int kq, struct epoll_event *events,  int nevents)
#else
void handle_event(int kq, struct kevent *events,  int nevents)
#endif
{
    int i=0;
    for(i=0; i<nevents; i++) {
        #ifdef __linux__
        void *udata = events[i].data.ptr;
        handle_epoll_event(kq, udata);
        #else
        int sock = events[i].ident;
        int data = events[i].data;
        void *udata = events[i].udata;
        if (sock == listen_socket) {
            handle_accept(kq, data);
        } else {
            handle_receive(kq, sock, data, udata);
        }
        #endif
    }
}

int main_loop(int kq)
{
#ifdef __linux__
    struct epoll_event events[MAX_EVENT_COUNT];
    int ret = 0;
    while(1) {
        ret = epoll_wait(kq, events, MAX_EVENT_COUNT, 1000);
        if (ret < 0) {
            printf("epoll wait error.\n");
            continue;
        }
        handle_event(kq, events, ret);
    }
#else
    struct kevent events[MAX_EVENT_COUNT];
    while(1) {
        ret = kevent(kq, NULL, 0, events, MAX_EVENT_COUNT, NULL);
        if (ret == -1) {
            printf("kevent failed\n");
            continue;
        }
        handle_event(kq, events, ret);
    }
#endif
}

int main(int argc, char *argv[])
{
    int client_queue = 0;
    int ret = 0;

#ifdef __linux__
    printf("this is linux server.\n");
#else
    printf("this may be freebsd.\n");
#endif

    conf_load("./tp.json", &opt);

    ret = server_start(&listen_socket, "0.0.0.0", opt.port);

#ifdef __linux__
    client_queue = epoll_create(MAX_EVENT_COUNT);
#else
    client_queue = kqueue();
#endif
    regist_sock(client_queue, listen_socket, NULL);

    main_loop(client_queue);

    return 0;
}
