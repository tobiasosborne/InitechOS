/* Factory oracle, initech-cqej. Ref: PRD Sec 8, qemu.h quit_after_count,
 * spec/flair_power_traces.mk: restart is complete only on the SECOND boot.
 * Include the driver so the oracle exercises its actual live-capture parser.
 */
#include "bochs.c"

static void check(int yes, const char *rule)
{
    if (!yes) {
        fprintf(stderr, "RED bochs-completion: %s\n", rule);
        exit(1);
    }
}

int main(void)
{
    check(serial_marker_count("READY\n", "READY") == 1, "first boot counted");
    check(serial_marker_count("READY\nRESET\nREADY\n", "READY") == 2,
          "second boot counted");
    check(serial_marker_count("NOT-READY\nREADY-extra\n", "READY") == 0,
          "marker substrings must not count as boot lines");
    check(serial_marker_count("READY", "READY") == 0,
          "partial serial lines must not count as boot lines");
    check(serial_marker_count("READY\r\nREADY\r\n", "READY") == 2,
          "CRLF serial counts both boots");
    check(!serial_completion("READY\n", "READY", 2),
          "one boot must not satisfy a two-boot wait");
    check(serial_completion("READY\nRESET\nREADY\n", "READY", 2),
          "second boot satisfies wait");
    puts("GREEN test-bochs-completion: exact complete lines, CRLF, second-boot wait");
    return 0;
}
