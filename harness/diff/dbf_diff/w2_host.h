#ifndef W2_HOST_H
#define W2_HOST_H
#include "samir/pal.h"
#include "samir/interp.h"
/* Factory inputs matching mint_capture.sh mounts; never host disk capacity. */
#define W2_HOST_PATH_CAP 4096
struct w2_host_config {
    const char *c_root, *d_root;
    uint32_t c_free_bytes, d_free_bytes;
    char logged_drive;
};
struct w2_host_env {
    samir_pal_t *pal;
    char roots[2][W2_HOST_PATH_CAP];
    uint32_t free_bytes[2];
    char logged_drive;
    unsigned input_calls;
    pal_fd (*open)(samir_pal_t *,const char *,int);
    int (*remove)(samir_pal_t *,const char *);
    int (*rename)(samir_pal_t *,const char *,const char *);
    int (*curdrive)(samir_pal_t *);
    int (*diskspace)(samir_pal_t *,uint32_t *);
    int32_t (*conin_line)(samir_pal_t *,char *,uint32_t);
    int32_t (*conin_line_noecho)(samir_pal_t *,char *,uint32_t);
};
int w2_host_bind(struct w2_host_env *,samir_pal_t *,const struct w2_host_config *);
void w2_host_unbind(struct w2_host_env *);
int w2_host_startup(samir_pal_t *,xb_interp *,const char *);
#endif
