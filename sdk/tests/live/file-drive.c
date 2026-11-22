/* Ordinary disk client. The harness grades the gate results and FAT bytes. */
#include "tbx.h"

const char tbx_app_name[] = "SDKDRIVE";

int tbx_app_main(void)
{
    static tbx_find_rec_t entry;
    static char result[] = "?:\r\n";

    (void)tbx_file_drive();
    if (tbx_register() != TBX_OK) return 1;
    (void)tbx_file_drive();
    if (tbx_file_find("A:SDKPROBE.TXT", 0, &entry, 1) != 1) {
        tbx_exit(2);
        return 2;
    }
    int32_t drive = tbx_file_drive();
    if (drive < 0 || drive > 25) {
        tbx_exit(3);
        return 3;
    }
    result[0] = (char)('A' + drive);
    int32_t h = tbx_file_create_new("SDKDRIVE.TXT");
    if (h < 5) {
        tbx_exit(4);
        return 4;
    }
    int32_t written = tbx_file_write(h, result, sizeof result - 1);
    int32_t closed = tbx_file_close(h);
    tbx_exit(written == sizeof result - 1 && closed == TBX_OK ? 0 : 5);
    (void)tbx_file_drive();
    return 0;
}

void tbx_app_event(const tbx_event_t *e) { (void)e; }
