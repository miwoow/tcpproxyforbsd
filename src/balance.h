#ifndef _BALANCE_H_
#define _BALANCE_H_

#include "server.h"

struct real_ip_pkg_s {
    uint32_t rip;
};

int report_real_client_ip(struct client *cli);
int balance_sel_server(int kq, struct client *cli);

#endif
