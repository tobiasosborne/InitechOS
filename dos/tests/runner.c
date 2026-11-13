/* Factory adapter: run a POSIX shell oracle and preserve its exit status. */
#include <unistd.h>
#include <stdio.h>
int main(int argc, char **argv) {
    if (argc < 2) return 2;
    argv[0] = "sh";
    execvp("sh", argv);
    perror("sh");
    return 2;
}
