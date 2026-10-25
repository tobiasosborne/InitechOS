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
#include <unistd.h>
static inline void scratch_lock(const char *path)
{
    struct flock l;
    int fd = open(path, O_CREAT | O_RDWR, 0666);
    if (fd < 0) return;                 /* no build dir yet: run unserialized */
    l.l_type = F_WRLCK; l.l_whence = SEEK_SET; l.l_start = 0; l.l_len = 0; l.l_pid = 0;
    while (fcntl(fd, F_SETLKW, &l) < 0 && errno == EINTR) { }
}
#endif
