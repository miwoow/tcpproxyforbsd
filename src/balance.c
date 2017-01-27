#include "balance.h"
#include "conf.h"
#include "server.h"

int upstream_queue = 0; // queue of socket to upstream
char buf_[MAX_RECV_BUFF];
extern struct conf_s opt;

int balance_sel_server(int kq, struct client *cli)
{
    int sk = socket(AF_INET, SOCK_STREAM, 0);
    int flags;
    int ret;
    struct sockaddr_in up_addr;
    int selidx; // index of selected server.

    selidx = (cli->client_addr).s_addr % opt.nsips;

    memset(&up_addr, 0, sizeof(struct sockaddr_in));
    up_addr.sin_family = AF_INET;
    up_addr.sin_port = htons(opt.sport);
    up_addr.sin_addr.s_addr = opt.sips[selidx];

    ret = connect(sk, (struct sockaddr *)&up_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        printf("connect error.\n");
        return -1;
    }

    cli->upstream_fd = sk;

    ret = regist_sock(kq, sk, cli);
    if (ret < 0) {
        printf("regist upstream client failed\n");
        return -1;
    }

    // send client real ip to upstream.
    ret = report_real_client_ip(cli);
    return ret;
}

int report_real_client_ip(struct client *cli)
{
    struct real_ip_pkg_s pkg = {0};
    int ret = 0;
    pkg.rip = cli->client_addr.s_addr;
    ret = send(cli->upstream_fd, &pkg, sizeof(struct real_ip_pkg_s), 0);
    if (ret < 0) {
        clear_client(cli);
        printf("send real ip error kick client\n");
        return -1;
    }
    return 0;
}
