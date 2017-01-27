#include "server.h"

int server_start(int *sk, char *ip, int port)
{
    struct sockaddr_in my_addr;
    int flags;
    int ret = 0;

    memset(&my_addr, 0, sizeof(struct sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = inet_addr(ip);

    *sk = socket(AF_INET, SOCK_STREAM, 0);

    flags = fcntl(*sk, F_GETFL, 0);
    fcntl(*sk, F_SETFL, flags | O_NONBLOCK);

    ret = bind(*sk, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        printf("error in bind\n");
        return -1;
    }
    ret = listen(*sk, 65535);
    if (ret < 0) {
        printf("error in listen\n");
        return -1;
    }
    return 0;
}

int regist_sock(int kq, int fd, struct client *cli)
{
    struct kevent changes[1];

    if (cli == NULL) {
        struct sockaddr_in peer_addr;
        socklen_t socklen = 0;

        cli = malloc(sizeof(struct client));
        memset(cli, 0, sizeof(struct client));
        memset(&peer_addr, 0, sizeof(struct sockaddr_in));
        socklen = sizeof(struct sockaddr_in);
        getpeername(fd, (struct sockaddr *)&peer_addr, &socklen);
        cli->fd = fd;
        cli->client_addr = peer_addr.sin_addr;
        cli->port = peer_addr.sin_port;
    }

    EV_SET(&changes[0], fd, EVFILT_READ, EV_ADD, 0, 0, cli);
    int ret = kevent(kq, changes, 1, NULL, 0, NULL);
    if (ret == -1) {
        printf("error in kevent\n");
        return -1;
    }
    return 0;
}

int clear_client(struct client *cli)
{
    if (cli->fd) {
        close(cli->fd);
    }

    if (cli->upstream_fd) {
        close(cli->upstream_fd);
    }

    free(cli);
    cli = NULL;
    return 0;
}
