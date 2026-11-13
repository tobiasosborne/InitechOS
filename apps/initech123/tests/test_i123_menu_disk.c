/* Shipped startup configuration, over sdk/host. Ground truth: legacy-final
 * face_a.c tbx_app_main opens A:\APPS\123.MNU, closes it, and EXIT(1)s
 * before publishing the menu bar/window on missing, read/close error or any
 * i123_menu_load rejection. core/menu.c pins framing, length and FNV checksum.
 * This is the authored resource policy, not a claim about Lotus's loader.
 * The input is the real generated resource; menu_tree independently compares
 * every word/edge against installed 123.RI and the captured tree. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "test_assert.h"
#include "tbx_host.h"
#include "../app/face_a.c"
TEST_HARNESS();
static uint8_t resource[65536];
static size_t resource_len;
static void key_text(const char *s)
{
    while (*s) {
        tbx_event_t e = {0};
        e.what = keyDown; e.message = (uint8_t)*s++;
        tbx_app_event(&e);
    }
}
static void exercise(int kind)
{
    tbx_host_reset();
    uint32_t len = (uint32_t)resource_len;
    if (kind == 1) len = 8;                /* truncated header */
    if (kind == 2) len--;                  /* truncated body/checksum */
    if (kind == 4) resource[0] ^= 1;        /* bad magic */
    if (kind == 5) resource[100] ^= 1;      /* valid header, bad checksum */
    if (kind != 3)
        CHECK(tbx_host_put_file("A:\\APPS\\123.MNU", resource, len), "install menu fixture");
    if (kind == 6) tbx_host.read_limit = 17; /* the loader must accumulate reads */
    if (kind == 7) tbx_host.file_error[TBX_FILE_READ] = 5;
    if (kind == 8) tbx_host.fail_close = 1;
    int rc = tbx_app_main();
    CHECK(tbx_host.registrations == 1, "register before resource access");
    if (kind == 0 || kind == 6) {
        CHECK(rc == 0 && tbx_host.exit_calls == 0, "valid resource starts");
        CHECK(tbx_host.nwindows == 1 && tbx_host.bar != NULL, "publish window and menu bar");
        tbx_event_t e = {0}; e.what = updateEvt; tbx_app_event(&e);
        key_text("/");
        CHECK(strstr(tbx_host.scr[1], "Worksheet  Range  Copy  Move  File") != NULL,
              "disk menu words appear on slash");
        key_text("f");
        CHECK(strstr(tbx_host.scr[1], "Retrieve  Save") != NULL, "File branch navigates");
        key_text("r");
        CHECK(strstr(tbx_host.scr[1], "Name of file to retrieve:") != NULL,
              "disk menu dispatch opens Retrieve prompt");
    } else {
        CHECK(rc == 1, "missing or invalid resource refuses startup");
        CHECK(tbx_host.exit_calls == 1 && tbx_host.exit_rc == 1, "resource failure EXIT(1)");
        CHECK(tbx_host.nwindows == 0 && tbx_host.bar == NULL, "failure publishes no window/menu");
    }
    for (unsigned h = 0; h < TBX_HOST_HANDLES; h++)
        CHECK(!tbx_host.handles[h].used, "resource handle closed");
}
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 2; }
    resource_len = fread(resource, 1, sizeof resource, f);
    if (ferror(f) || fclose(f) || resource_len < 16) return 2;
    for (int kind = 0; kind < 9; kind++) {
        int fd[2], counts[2] = {0};
        if (pipe(fd)) return 2;
        fflush(NULL);
        pid_t pid = fork();
        if (pid < 0) return 2;
        if (!pid) {
            close(fd[0]); g_checks = g_fails = 0; exercise(kind);
            counts[0] = g_checks; counts[1] = g_fails;
            if (write(fd[1], counts, sizeof counts) != sizeof counts) _exit(2);
            _exit(0);
        }
        close(fd[1]);
        ssize_t n = read(fd[0], counts, sizeof counts); close(fd[0]);
        int status = 0;
        CHECK(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
              "fresh application process completes");
        CHECK(n == sizeof counts, "child reports all assertions");
        if (n == sizeof counts) { g_checks += counts[0]; g_fails += counts[1]; }
    }
    return TEST_SUMMARY("menu_disk");
}
