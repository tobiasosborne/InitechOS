/* initech-tbbri: Bochs journey waits consume markers in order, as whole names,
 * on complete lines, exactly like the QEMU W waits (emu/qemu.c). */
#define main desktop_driver_main
#include "desktop-bochs.c"
#undef main
static FILE *serial;
static int bad;
static void emit(const char *text) { fputs(text, serial); fflush(serial); }
static void expect(const char *marker, int want, const char *why)
{
    BochsTraceStep step;
    char spec[200];
    snprintf(spec, sizeof spec, "W%s", marker);
    int got = journey_parse(spec, strlen(spec), &step) && step.kind == BX_WAIT;
    if (got != want) { printf("FAIL Bochs marker: %s\n", why); bad = 1; }
}
int main(void)
{
    BochsTraceStep step;
    if (!journey_parse("Wx", 2, &step)) return 2; /* leave the validation pass */
    serial = tmpfile();
    if (!serial) return 2;
    char path[64];
    snprintf(path, sizeof path, "/proc/self/fd/%d", fileno(serial));
    if (setenv("DESKTOP_SERIAL", path, 1) || setenv("DESKTOP_WAIT_MS", "300", 1)) return 2;
    emit("FLAIR-DISPATCH app=CALCULAT\n");
    expect("FLAIR-DISPATCH app=CALC", 0, "app=CALC satisfied by app=CALCULAT");
    emit("TENANT-EXIT rc=0 via=exit\n");
    expect("TENANT-EXIT rc=0 via=exit", 1, "first exit not accepted");
    expect("TENANT-EXIT rc=0 via=exit", 0, "second exit satisfied by the first exit's line");
    emit("FLAIR-TENANT-EXIT rc=0 via=exit\n");
    expect("TENANT-EXIT rc=0 via=exit", 0, "TENANT-EXIT matched inside FLAIR-TENANT-EXIT");
    emit("FINDER-OPEN-VOLUME\nFLAIR-DISPATCH app=WORD\n");
    expect("FLAIR-DISPATCH app=WORD", 1, "later marker not accepted");
    expect("FINDER-OPEN-VOLUME", 0, "marker printed before the previous wait accepted");
    emit("TENANT-GATE s=\"A:\\WORD.WP5\" ok\n");
    expect("TENANT-GATE s=\"A:\\WORD.WP5", 1, "deliberate quoted prefix refused");
    emit("FLAIR-DISPATCH app=NOTEPAD");
    expect("FLAIR-DISPATCH app=NOTEPAD", 0, "incomplete line accepted");
    if (bad) return 1;
    puts("ok Bochs markers: consumed in order as whole names on complete lines");
    return 0;
}
