/* initech-nstg; PRD Sec 8 + CLAUDE.md Rule 2. Host-only preflight test.
 * Include the implementation to exercise argv construction without QEMU.
 * Trap fork too: even the missing-preflight mutant must stay host-only. */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <sys/types.h>
static pid_t paths_test_fork(void);
#define fork paths_test_fork
#include "qemu.c"
#undef fork
static pid_t paths_test_fork(void) { errno=EAGAIN; return -1; }

static int reject(const char *dir, int stdio_mode, const char *why)
{
    QemuConfig cfg = { .disk_path = "unused.img", .output_dir = dir,
        .name = "paths", .keys_spec = "ret", .quit_after = "done" };
    QemuResult result;
    if (stdio_mode) setenv("INITECH_QMP_STDIO", "1", 1);
    else unsetenv("INITECH_QMP_STDIO");
    FILE *capture = tmpfile();
    if (!capture) return 1;
    int saved = dup(STDERR_FILENO);
    dup2(fileno(capture), STDERR_FILENO);
    int rc = qemu_run(&cfg, &result);
    fflush(stderr);
    dup2(saved, STDERR_FILENO);
    close(saved);
    rewind(capture);
    char diagnostic[512] = {0};
    (void)fread(diagnostic, 1, sizeof(diagnostic)-1, capture);
    fclose(capture);
    int bad = rc == 0 || result.launched || !strstr(diagnostic, why);
    qemu_result_free(&result);
    if (bad) fprintf(stderr, "RED qemu-paths: %s must fail loudly BEFORE launch\n", why);
    return bad;
}

int main(void)
{
    char dir[QEMU_PATH_MAX + 80];
    memset(dir, 'x', sizeof(dir)); dir[0] = '/'; dir[180] = 0;
    if (reject(dir, 0, "QMP socket path too long")) return 1;
    /* Same long absolute --out is valid over stdio; nothing is truncated. */
    char serial[QEMU_PATH_MAX], log[QEMU_PATH_MAX], sock[QEMU_PATH_MAX];
    char *argv[MAX_ARGV];
    QemuConfig cfg = { .disk_path="disk.img", .output_dir=dir,
        .keys_spec="ret" };
    setenv("INITECH_QMP_STDIO", "1", 1);
    if (join_path(serial,sizeof(serial),dir,"paths",".serial") ||
        join_path(log,sizeof(log),dir,"paths",".log") ||
        join_path(sock,sizeof(sock),dir,"paths",".sock") ||
        build_argv(&cfg,argv,serial,log,sock,NULL) < 0) return 1;
    int found = 0;
    for (int i=0; argv[i]; i++)
        if (!strcmp(argv[i],"-serial") && argv[i+1] &&
            !strcmp(argv[i+1]+5,serial)) found = 1;
    if (!found) { fputs("RED qemu-paths: long stdio serial truncated\n",stderr); return 1; }
    memset(dir+1, 'x', sizeof(dir)-2); dir[sizeof(dir)-1]=0;
    if (reject(dir, 1, "output path too long")) return 1;
    puts("GREEN test-qemu-paths: absolute paths preserved in stdio; oversized output/socket rejected before launch");
    return 0;
}
