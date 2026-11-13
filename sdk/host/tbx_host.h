/* Factory-only recording Toolbox. See README.md for fixture semantics. */
#ifndef INITECH_TBX_HOST_H
#define INITECH_TBX_HOST_H
#include "tbx.h"
#define TBX_HOST_W 76
#define TBX_HOST_H 24
#define TBX_HOST_OPS 16384
#define TBX_HOST_FILES 64
#define TBX_HOST_HANDLES 32
#define TBX_HOST_EVENTS 256
/* Every call records its binding name, scalar arguments and a text snapshot.
 * Pointer-bearing records remain native host pointers in the typed state. */
typedef struct tbx_host_op {
    const char *name;
    int32_t a[6];
    char text[256];
} tbx_host_op_t;
typedef struct tbx_host_file {
    char name[TBX_FILE_PATH_MAX];
    uint8_t *data;
    uint32_t size, capacity;
    uint8_t attr;
    uint16_t time, date;
    int used;
} tbx_host_file_t;
typedef struct tbx_host_state {
    char scr[TBX_HOST_H][TBX_HOST_W+1], bg[TBX_HOST_H][TBX_HOST_W+1];
    char inv[TBX_HOST_H][TBX_HOST_W];
    uint8_t face[TBX_HOST_H][TBX_HOST_W], col[TBX_HOST_H][TBX_HOST_W];
    tbx_host_op_t ops[TBX_HOST_OPS];
    unsigned nops, dropped_ops, registrations, beeps, clock_reads;
    int exit_calls, exit_rc, alerts, setscroll, track, sf, menubar_draws;
    int cw, ch, origin_h, origin_v, nwindows;
    struct { int32_t l,t,r,b,goaway; char title[128]; int scroll_len,scroll_val; } windows[TBX_TENANT_WINDOWS];
    char alert_text[256], alert_btn[3][32];
    int32_t alert_rc, clock_rc, dialog_result;
    cp_time_t clock;
    cp_settings_t settings;
    uint32_t memory[4];
    const cp_item_t *dialog_items;
    uint32_t dialog_count, dialog_def, dialog_cancel;
    tbx_event_t dialog_event;
    const tbx_menubar_t *bar;
    char sf_prompt[64], sf_name[128], *sf_reply;
    char copy_caption[128], copy_src[128], copy_dst[128];
    uint8_t scrap[8192];
    int scrap_len;
    unsigned scrap_puts, find_calls;
    int32_t free_bytes;
    int fail_create, fail_write;
    uint32_t read_limit, write_limit;
    int64_t write_budget; /* -1 unlimited; 0 disk full */
    int fail_close, fail_close_handle; /* one close error; 0 selects any */
    /* Persistent DOS error code by operation, e.g. FILE_CLOSE -> 5.
     * Zero means normal behavior. All file verbs consult this array. */
    uint8_t file_error[256];
    tbx_host_file_t files[TBX_HOST_FILES];
    char deleted[TBX_HOST_FILES*2][TBX_FILE_PATH_MAX];
    unsigned ndeleted;
    struct { int used, file; uint32_t pos, mode; } handles[TBX_HOST_HANDLES];
    tbx_event_t events[TBX_HOST_EVENTS];
    unsigned event_head, event_count;
} tbx_host_state_t;
extern tbx_host_state_t tbx_host;
void tbx_host_reset(void);
int tbx_host_put_file(const char *path, const void *bytes, uint32_t size);
const tbx_host_file_t *tbx_host_file(const char *path);
int tbx_host_push_event(const tbx_event_t *event);
/* Delivers queued callbacks; each callback return is a yield. */
unsigned tbx_host_dispatch(void (*callback)(const tbx_event_t *));
/* Explicit disk fixture adapters, never used by the fake's file verbs. */
int tbx_host_import(const char *directory);
int tbx_host_export(const char *directory);
#endif
