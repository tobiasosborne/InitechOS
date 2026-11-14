/* Factory timing only: never linked into the product or used in output bytes. */
#include <stdio.h>
#include <time.h>

int main(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        perror("CLOCK_MONOTONIC");
        return 1;
    }
    return printf("%lld\n", (long long)now.tv_sec * 1000
                           + now.tv_nsec / 1000000) < 0;
}
