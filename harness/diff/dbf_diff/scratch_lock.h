/* scratch_lock.h -- serialize test binaries that share a fixed scratch dir.
 *
 * initech-2kkqr (F2): several SAMIR oracles (and their mutant legs) stage
 * files under one fixed build/<dir>; run concurrently under `make -j` they
 * clobber each other's images/logs and go falsely RED. Each such test takes an
 * exclusive POSIX record lock on build/.lock-<name> for its whole run (the
 * lock dies with the process, even on abort). The tests are unchanged
 * otherwise -- same inputs, same checks, same verdicts; they just queue.
 * ASCII only; POSIX fcntl, no flock(2) (hidden under -D_POSIX_C_SOURCE). */
#ifndef SCRATCH_LOCK_H
#define SCRATCH_LOCK_H
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static inline void scratch_lock_error(const char *path)
{
    fprintf(stderr, "RED scratch lock %s: %s\n", path, strerror(errno));
    exit(2);
}
static inline void scratch_lock(const char *path)
{
    /* Ref: CLAUDE Rules 2/11. Exclusion is a precondition, never optional. */
    struct flock l;
    char parent[4096];
    size_t i, n = strlen(path);
    int fd;
    if (n >= sizeof parent) { errno = ENAMETOOLONG; scratch_lock_error(path); }
    memcpy(parent, path, n + 1);
    for (i = 1; i < n; i++) if (parent[i] == '/') {
        parent[i] = 0;
        if (mkdir(parent, 0755) && errno != EEXIST) scratch_lock_error(path);
        parent[i] = '/';
    }
    fd = open(path, O_CREAT | O_RDWR, 0666);
    if (fd < 0) scratch_lock_error(path);
    l.l_type = F_WRLCK; l.l_whence = SEEK_SET; l.l_start = 0; l.l_len = 0; l.l_pid = 0;
    while (fcntl(fd, F_SETLKW, &l) < 0) {
        if (errno != EINTR) scratch_lock_error(path);
    }
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) scratch_lock_error(path);
}
#endif
