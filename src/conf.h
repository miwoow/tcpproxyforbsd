#ifndef _CONF_H_
#define _CONF_H_

#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_SIPS 128

struct conf_s {
    int port;
    int sport;
    char method[32];
    uint32_t sips[MAX_SIPS];
    int nsips;
};

int conf_load(char *fpath, struct conf_s *opt);

#endif

