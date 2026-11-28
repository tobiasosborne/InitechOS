/* Host-only adapter for the Bochs 2.7 RFB plugin (gui/rfb.cc).
 * Its listener passes INADDR_ANY and a hard-coded port to bind(2). Ask the
 * kernel for a free loopback port on that same socket, then publish it to
 * the harness through this child's log. Bochs continues with listen(2). */
#include "rfb_port.h"
#include <dlfcn.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int bind(int fd, const struct sockaddr *addr, socklen_t len)
{
    int (*host_bind)(int, const struct sockaddr *, socklen_t) = dlsym(RTLD_NEXT, "bind");
    if (!host_bind) {
        errno = ENOSYS;
        return -1;
    }
    if (!addr || len != sizeof(struct sockaddr_in) || addr->sa_family != AF_INET)
        return host_bind(fd, addr, len);
    struct sockaddr_in endpoint;
    memcpy(&endpoint, addr, sizeof endpoint);
    unsigned port = ntohs(endpoint.sin_port);
    if (endpoint.sin_addr.s_addr != htonl(INADDR_ANY) || port < 5900 || port > 5949)
        return host_bind(fd, addr, len);

    endpoint.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    endpoint.sin_port = 0;
    int exclusive = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &exclusive, sizeof exclusive) < 0 ||
        host_bind(fd, (const struct sockaddr *)&endpoint, len) < 0)
        return -1;
    if (getsockname(fd, (struct sockaddr *)&endpoint, &len) < 0 ||
        dprintf(STDERR_FILENO, BOCHS_RFB_PORT_PREFIX "%u\n", ntohs(endpoint.sin_port)) < 0) {
        perror("Bochs RFB port publication");
        _exit(1);
    }
    return 0;
}
