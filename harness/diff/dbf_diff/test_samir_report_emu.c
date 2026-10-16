/* test_samir_report_emu.c -- REPORT FORM inside InitechOS (initech-airx).
 *
 * REAL-dBASE DIFFERENTIAL on the target (Law 2, Rule 14): boot the OS, EXEC
 * SAMIR.COM from COMMAND.COM, type USE / REPORT FORM ... TO FILE, QUIT, then
 * read the written files back off the FAT volume with mtools (an independent
 * reader) and compare them byte for byte with the files real DBASE.EXE 1.1
 * wrote for the same commands (mint session 004: G1FLT/G2TRV/G8TP/G9TS,
 * ../dbase3-decomp/re/mint-results-004.md; md5-pinned by the Makefile).
 *
 * The screen leg types REPORT FORM TRAVELER NEXT 27 -- the command of the
 * real screenshot R4S3.PNG, where the page eject is invisible and "Page No.
 * 2" follows "Pope" on the next row -- and checks the final 80x25 screendump
 * cell by cell (ink vs blank) against the console projection of the REAL
 * G2TRV.TXT bytes (FF removed, cut after the 27th record), never against
 * SAMIR output. SAMIR is a text-console app: this screendump is its clip.
 *
 * Usage: test_samir_report_emu files|screen [SAMIR.COM] [corpus]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_assert.h"
TEST_HARNESS();

#define D "build/samir-report-emu"
static char serial[1 << 20];

static int command(const char *s)
{
    int rc = system(s);
    CHECK(rc == 0, s);
    return rc;
}
static long slurp(const char *path, char *out, long cap)
{
    FILE *f = fopen(path, "rb");
    long n = 0;
    if (f) { n = (long)fread(out, 1, (size_t)cap - 1u, f); fclose(f); }
    out[n] = 0;
    return f ? n : -1;
}
/* QEMU sendkey names for the typed command text. */
static void keys(const char *t, char *out, size_t cap)
{
    size_t n = 0;
    for (; *t; t++) {
        char one[2] = { *t, 0 };
        const char *k = *t == '\n' ? "ret" : *t == ' ' ? "spc" : *t == '.' ? "dot" :
                        *t == '_' ? "shift-minus" : *t == '-' ? "minus" :
                        ((*t >= 'a' && *t <= 'z') || (*t >= '0' && *t <= '9')) ? one : NULL;
        CHECK(k != NULL, "key script character supported");
        if (!k) exit(2);
        if (n) out[n++] = ',';
        CHECK(n + strlen(k) + 1 < cap, "key script fits");
        strcpy(out + n, k); n += strlen(k);
    }
    out[n] = 0;
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "files";
    const char *com = argc > 2 ? argv[2] : "build/SAMIR.COM";
    const char *corpus = argc > 3 ? argv[3] : "../dbase3-decomp";
    int screen = !strcmp(mode, "screen"), i;
    char cmd[12288], keybuf[6144], path[1024];
    static char got[1 << 16], want[1 << 16], report[8192];

    if (command("mkdir -p " D)) return 2;
    snprintf(cmd, sizeof cmd, "cp -f build/samir_report_data.img " D "/%s.img && mcopy -o -i " D "/%s.img %s ::SAMIR.COM", mode, mode, com);
    if (command(cmd)) return 2;
    snprintf(cmd, sizeof cmd, "fsck.fat -n " D "/%s.img > " D "/%s-fsck-before.log", mode, mode);
    command(cmd);
    if (screen) {
        /* R4S3.PRG verbatim: USE ... INDEX TRIPS (default .NDX, initech-vfys) leaves the
         * first record in index order current (initech-k7lc; mint 005), so no GO TOP. */
        keys("samir\nuse travel index trips\nreport form traveler next 27\n", keybuf, sizeof keybuf);
        snprintf(cmd, sizeof cmd, "build/qemu_harness --disk build/tracer_secondary.img --disk2 " D "/screen.img --name samir_report_screen --out build --rtc-base 1986-07-24T12:00:00 --timeout-ms 150000 --keys '%s' --keys-after SHELL-READY --screendump --screendump-after \"$(printf '1779.00 .T.\\r\\n. ')\" > " D "/screen.out 2> " D "/screen.report", keybuf);
        /* Grab once the last record (Arlich, the first 1779.00 row) is
         * followed by the dot prompt: SAMIR then idles on input, screen final. */
        command(cmd);
    } else {
        /* Commands arrive on redirected stdin (the K13 PAL line framer): keys
         * typed while a long report renders would overrun the keyboard. */
        FILE *f = fopen(D "/RPT.TXT", "wb");
        const char *rpt = "use aval_flt index aval_flt\r\nreport form flt_info to file g1flt\r\n"
                          "use travel index trips\r\nreport form traveler to file g2trv\r\n"
                          "use r4trip\r\nreport form trip to file g8tp\r\nreport form trip summary to file g9ts\r\nquit\r\n";
        CHECK(f != NULL && fwrite(rpt, 1, strlen(rpt), f) == strlen(rpt), "command file written");
        if (f) fclose(f);
        command("mcopy -o -i " D "/files.img " D "/RPT.TXT ::RPT.TXT");
        f = fopen(D "/RPT.BAT", "wb");   /* '<' is not a send-key token */
        CHECK(f != NULL && fputs("samir < rpt.txt\r\n", f) >= 0, "batch file written");
        if (f) fclose(f);
        command("mcopy -o -i " D "/files.img " D "/RPT.BAT ::RPT.BAT");
        keys("rpt\nexit\n", keybuf, sizeof keybuf);
        snprintf(cmd, sizeof cmd, "build/qemu_harness --disk build/tracer_secondary.img --disk2 " D "/files.img --name samir_report_files --out build --rtc-base 1986-07-24T12:00:00 --timeout-ms 120000 --keys '%s' --keys-after SHELL-READY --quit-after SHELL-DONE > " D "/files.out 2> " D "/files.report", keybuf);
        command(cmd);
    }
    snprintf(path, sizeof path, D "/%s.report", mode);
    slurp(path, report, sizeof report);
    CHECK(strstr(report, "triple_fault=0") != NULL, "guest did not triple fault");
    {
        const char *s = strstr(report, "keys_sent=");
        unsigned expected = 1;
        const char *k = keybuf;
        while (*k) if (*k++ == ',') expected++;
        CHECK(s && strtoul(s + 10, NULL, 10) == expected, "entire input trace injected");
    }
    snprintf(path, sizeof path, "build/samir_report_%s.serial", mode);
    {
        long n = slurp(path, serial, sizeof serial), j = 0;
        for (i = 0; i < n; i++) if (serial[i] != '\r') serial[j++] = serial[i];
        serial[j] = 0;
    }
    CHECK(strstr(serial, "SHELL-READY") != NULL, "OS shell booted");
    CHECK(strstr(serial, "PC LOAD LETTER") == NULL, "no guest panic");
    CHECK(strstr(serial, "Unrecognized") == NULL && strstr(serial, "07/24/86") != NULL && strstr(serial, "Report file invalid") == NULL &&
          strstr(serial, "File does not exist") == NULL, "REPORT FORM accepted in the guest");
    if (!screen) {
        static const char *const out[] = { "G1FLT.TXT", "G2TRV.TXT", "G8TP.TXT", "G9TS.TXT" };
        CHECK(strstr(serial, "SHELL-DONE") != NULL, "SAMIR quit and the shell returned");
        for (i = 0; i < 4; i++) {
            long ng, nw;
            snprintf(cmd, sizeof cmd, "mcopy -o -n -i " D "/files.img ::%s " D "/%s", out[i], out[i]);
            command(cmd);
            snprintf(path, sizeof path, D "/%s", out[i]);
            ng = slurp(path, got, sizeof got);
            snprintf(path, sizeof path, "%s/mint/work/%s", corpus, out[i]);
            nw = slurp(path, want, sizeof want);
            CHECK(nw > 0, "real dBASE golden present");
            if (ng != nw || memcmp(got, want, (size_t)nw)) {
                long k = 0;
                while (k < ng && k < nw && got[k] == want[k]) k++;
                fprintf(stderr, "REPORT %s: guest file differs from real dBASE at byte %ld (got %ld, want %ld bytes)\n", out[i], k, ng, nw);
            }
            printf("  %-10s %s\n", out[i], ng == nw && !memcmp(got, want, (size_t)nw) ? "guest file byte-identical to real dBASE III PLUS 1.1" : "DIFFERS");
            CHECK(ng == nw && !memcmp(got, want, (size_t)nw), "REPORT FORM TO FILE in the guest byte-identical to real dBASE III PLUS 1.1");
        }
        CHECK(strstr(serial, "    ** Travel Code AO26") != NULL, "guest console shows the TRAVELER group band");
        CHECK(strstr(serial, "       *** Total ***\n                                              152281.9") != NULL,
              "guest console shows the TRIP grand total band");
        command("fsck.fat -n " D "/files.img > " D "/files-fsck-after.log");
    } else {
        /* Cell manifest from the REAL golden: console projection, last 24 rows. */
        long nw, k = 0, s, e;
        char *line[512];
        int nl = 0, row;
        FILE *m;
        snprintf(path, sizeof path, "%s/mint/work/G2TRV.TXT", corpus);
        nw = slurp(path, want, sizeof want);
        CHECK(nw > 0, "real G2TRV golden present");
        /* Drop the eject (CR FF) and the closing ^Z: what the screen shows. */
        for (s = 0; s < nw; s++)
            if (want[s] != '\f' && want[s] != 0x1a && !(want[s] == '\r' && s + 1 < nw && want[s + 1] == '\f'))
                got[k++] = want[s];
        got[k] = 0;
        {
            char *a = strstr(got, "     Arlich");
            CHECK(a != NULL, "golden has the 27th record (Arlich) to cut at");
            if (!a) return 2;
            e = strchr(a, '\r') ? strchr(a, '\r') - got : (long)strlen(got);
            got[e] = 0;
        }
        for (s = 0; s < e && nl < 512; ) {   /* lines are CR LF + text */
            char *t = strstr(got + s, "\r\n");
            if (!t) break;
            *t = 0; *(t + 1) = 0;
            s = (t - got) + 2;
            line[nl++] = got + s;
        }
        for (i = 0; i < nl; i++) { char *t = strstr(line[i], "\r\n"); if (t) *t = 0; }
        CHECK(nl > 30, "console projection has more than one screen of lines");
        m = fopen(D "/screen.cells", "w");
        CHECK(m != NULL, "cell manifest opened");
        if (!m) return 2;
        fprintf(m, "# rows 0-23 = last 24 console lines of real G2TRV.TXT (FF dropped), cut after Arlich\n");
        for (row = 0; row < 24; row++) {
            const char *t = line[nl - 24 + row];
            char padded[81];
            memset(padded, ' ', 80); padded[80] = 0;
            memcpy(padded, t, strlen(t) > 80 ? 80 : strlen(t));
            fprintf(m, "%d 0 %s\n", row, padded);
        }
        fclose(m);
        CHECK(!strcmp(line[nl - 1], "     Arlich               Kim                  11/23/85     1779.00 .T.") &&
              !strncmp(line[nl - 15], "     Page No.     2", 19) && !strncmp(line[nl - 16], "     Pope", 9), "manifest: page-2 header follows Pope with no FF row (as R4S3.PNG)");
        CHECK(system("build/ppm_text_check --cells build/samir_report_screen.ppm " D "/screen.cells") == 0,
              "guest screendump rows 0-23 match the real report text cell for cell");
    }
    snprintf(cmd, sizeof cmd, "fsck.fat -n " D "/%s.img > /dev/null", mode);
    command(cmd);
    return TEST_SUMMARY(screen ? "test-samir-report-emu (screen)" : "test-samir-report-emu (files)");
}
