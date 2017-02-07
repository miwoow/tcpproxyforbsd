#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
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
    struct client *cli = (struct client *)udata;
    int ret = 0;

    memset(buf_, 0, MAX_RECV_BUFF);
    int bytes = recv(sock, buf_, avail_bytes, 0);
    char *pos = NULL;
    int len = 0;
    if (sock == cli->fd) {
        // data from client
        if (bytes == 0 || bytes == -1) {
            printf("%s:%d: client close or recv failed.\n", inet_ntoa(cli->client_addr), ntohs(cli->port));
            clear_client(cli);
            cli = NULL;
            return -1;
        }
        printf("%s:%d: %s", inet_ntoa(cli->client_addr), ntohs(cli->port), buf_);


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
            ret = send(cli->upstream_fd, pos, len, 0);
            if (ret < 0) {
                clear_client(cli);
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
    } else if (sock == cli->upstream_fd) {
        // data from upstream
        if (bytes == 0 || bytes == -1) {
            printf("%s:%d: upstream close or recv failed.\n", inet_ntoa(cli->upstream_addr), cli->upstream_port);
            clear_client(cli);
            cli = NULL;
            return -1;
        }
        printf("%s:%d: %s", inet_ntoa(cli->client_addr), ntohs(cli->port), buf_);

        pos = &buf_[0];
        len = avail_bytes;
        while(1) {
            ret = send(cli->fd, pos, len, 0);
            if (ret < 0) {
                printf("send to client failed\n");
                clear_client(cli);
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

void handle_event(int kq, struct kevent *events,  int nevents)
{
    int i=0;
    for(i=0; i<nevents; i++) {
        int sock = events[i].ident;
        int data = events[i].data;
        void *udata = events[i].udata;
        if (sock == listen_socket) {
            handle_accept(kq, data);
        } else {
            handle_receive(kq, sock, data, udata);
        }
    }
}

int main_loop(int kq)
{
    struct kevent events[MAX_EVENT_COUNT];
    while(1) {
        int ret = kevent(kq, NULL, 0, events, MAX_EVENT_COUNT, NULL);
        if (ret == -1) {
            printf("kevent failed\n");
            continue;
        }
        handle_event(kq, events, ret);
    }
}

int main(int argc, char *argv[])
{
    int client_queue = 0;
    int ret = 0;

#ifdef __LINUX__
    printf("this is linux server.\n");
#endif

    conf_load("./tp.json", &opt);

    ret = server_start(&listen_socket, "0.0.0.0", opt.port);

    client_queue = kqueue();
    regist_sock(client_queue, listen_socket, NULL);

    main_loop(client_queue);

    return 0;
}
