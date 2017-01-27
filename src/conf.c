#include "conf.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int conf_load(char *fpath, struct conf_s *opt)
{
    json_object *jroot = json_object_from_file(fpath);
    json_object *jobj = NULL;
    json_bool json_ret = FALSE;

    json_ret = json_object_object_get_ex(jroot, "port", &jobj);
    if (json_ret) {
        opt->port = json_object_get_int(jobj);
        printf("port: %d\n", opt->port);
    }

    json_ret = json_object_object_get_ex(jroot, "sport", &jobj);
    if (json_ret) {
        opt->sport = json_object_get_int(jobj);
        printf("sport: %d\n", opt->sport);
    }

    json_ret = json_object_object_get_ex(jroot, "sips", &jobj);
    int i=0;
    const char *sip;
    if (json_ret) {
        opt->nsips = json_object_array_length(jobj);
        if (opt->nsips > MAX_SIPS) {
            printf("[WARN] sips num is too large, use MAX_SIPS: %d\n", MAX_SIPS);
            opt->nsips = MAX_SIPS;
        }
        for(i=0; i < opt->nsips; i++) {
            sip = json_object_get_string(json_object_array_get_idx(jobj, i));
            printf("sip%d: %s\n", i, sip);
            opt->sips[i] = inet_addr(sip);
        }
    }

    return 0;
}

