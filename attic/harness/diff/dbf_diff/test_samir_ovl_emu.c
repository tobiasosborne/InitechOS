/* test_samir_ovl_emu.c -- SAMIR.OVL refusal, in the guest (initech-5etom.1;
 * docs/design/samir-overlays.md Sec 8).
 *
 * Boots the DOS route (the same serial + real EXEC pattern as
 * test_samir_core_emu.c), puts SAMIR.EXE on a fresh 1.44 MB FAT disk with a
 * SAMIR.OVL that is
 *   ok       the build's own (positive control: the dot prompt is reached),
 *   missing  absent,
 *   stale    another build's, self-consistent but stamped differently
 *            (argv[3]: e.g. build/SAMIR_REPORT_subtotal.OVL),
 *   corrupt  the build's own with one byte flipped inside overlay 0's body,
 * and runs OVL.BAT: `samir`, then `if errorlevel 1 echo SAMIR-REFUSED-1`.
 * In the three refusal modes the operator presses ENTER (retry: the prompt
 * must come back) and then Ctrl-C (abort: return code 1, back at the DOS
 * prompt); in ok mode the dot prompt runs `? 'ovl'+'-ok'` and QUIT.
 *
 * The expected text is the LOCKED prompt (spec/samir/ovl_format.h), whose
 * wording is modelled on DBASE.EXE's AH=09h string at file offset 0x186E0.
 *
 * Usage: test_samir_ovl_emu ok|missing|stale|corrupt SAMIR.EXE [STALE.OVL]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "test_assert.h"
#include "samir_install.h"
#include "trace_complete.h"
#include "scratch_lock.h"
#include "samir/ovl_format.h"
TEST_HARNESS();
#define D "build/samir-ovl-emu"
static int command(const char *s){int rc=system(s);CHECK(rc==0,s);return rc;}
static unsigned count(const char *hay, const char *needle)
{
    unsigned n = 0;
    for (const char *p = strstr(hay, needle); p; p = strstr(p + 1, needle)) n++;
    return n;
}
int main(int argc, char **argv)
{
    scratch_lock("build/.lock-samir-ovl-emu");
    const char *mode = argc > 1 ? argv[1] : "ok", *exe = argc > 2 ? argv[2] : "build/SAMIR.EXE";
    const char *stale = argc > 3 ? argv[3] : "build/SAMIR_REPORT_subtotal.OVL";
    int ok = !strcmp(mode, "ok"), missing = !strcmp(mode, "missing");
    int corrupt = !strcmp(mode, "corrupt"), st = !strcmp(mode, "stale");
    char ovl[1024], cmd[8192], img[256];
    static char serial[262144];
    const char *prompt = "Insert System Disk and press ENTER, or press Ctrl-C to abort.";
    FILE *f;
    size_t n;

    if (!(ok || missing || corrupt || st) || samir_ovl_path(exe, ovl, sizeof ovl)) {
        fputs("usage: test_samir_ovl_emu ok|missing|stale|corrupt X.EXE [STALE.OVL]\n", stderr);
        return 2;
    }
    CHECK(strstr(SAMIR_OVL_PROMPT, prompt) != NULL, "locked prompt text is the one this oracle looks for");
    snprintf(img, sizeof img, D "/%s.img", mode);
    command("mkdir -p " D);
    f = fopen(img, "wb");
    CHECK(f != NULL, "FAT image allocated");
    if (!f) return 2;
    CHECK(fseek(f, 1474560 - 1, SEEK_SET) == 0 && fputc(0, f) != EOF, "FAT image sized");
    fclose(f);
    snprintf(cmd, sizeof cmd, "mformat -i %s -f 1440 :: && mcopy -o -i %s %s ::SAMIR.EXE", img, img, exe);
    if (command(cmd)) return 2;
    if (ok) {
        snprintf(cmd, sizeof cmd, "mcopy -o -i %s %s ::SAMIR.OVL", img, ovl);
        if (command(cmd)) return 2;
    } else if (st) {
        snprintf(cmd, sizeof cmd, "cmp -s %s %s", stale, ovl);
        CHECK(system(cmd) != 0, "the stale SAMIR.OVL differs from this build's");
        snprintf(cmd, sizeof cmd, "mcopy -o -i %s %s ::SAMIR.OVL", img, stale);
        if (command(cmd)) return 2;
    } else if (corrupt) {
        /* Flip one byte 64 bytes into overlay 0's module: the header, the
         * directory and the stamp are untouched, so only a CRC sees it. */
        unsigned char hdr[48];
        uint32_t off;
        {
            char copy[2048];
            snprintf(copy, sizeof copy, "cp -f %s " D "/CORRUPT.OVL", ovl);
            if (command(copy)) return 2;
        }
        f = fopen(D "/CORRUPT.OVL", "r+b");
        CHECK(f != NULL && fread(hdr, 1, sizeof hdr, f) == sizeof hdr, "overlay file header read");
        if (!f) return 2;
        off = (uint32_t)hdr[32] | ((uint32_t)hdr[33] << 8) | ((uint32_t)hdr[34] << 16) | ((uint32_t)hdr[35] << 24);
        CHECK(fseek(f, (long)off + 64, SEEK_SET) == 0, "seek into overlay 0");
        {
            int c = fgetc(f);
            CHECK(c != EOF && fseek(f, (long)off + 64, SEEK_SET) == 0 && fputc(c ^ 0x5A, f) != EOF, "one byte flipped");
        }
        fclose(f);
        snprintf(cmd, sizeof cmd, "mcopy -o -i %s " D "/CORRUPT.OVL ::SAMIR.OVL", img);
        if (command(cmd)) return 2;
    }
    f = fopen(D "/OVL.BAT", "wb");
    if (!f) return 2;
    fputs("@echo off\r\nsamir\r\nif errorlevel 1 echo SAMIR-REFUSED-1\r\n", f);
    fclose(f);
    snprintf(cmd, sizeof cmd, "mcopy -o -i %s " D "/OVL.BAT ::OVL.BAT", img);
    if (command(cmd)) return 2;
    /* Keys in stream order (--mouse k<chord>), each answer only once SAMIR
     * waits for it: typed ahead, a Ctrl-C would meet InitechDOS's BREAK=ON
     * check during SAMIR's start-up reads instead (INT 23h, "Terminate batch
     * job"), as on DOS. W<marker> holds until the prompt is on serial; the
     * retry (ENTER) re-opens and re-verifies SAMIR.OVL, given 3 s. */
    snprintf(cmd, sizeof cmd,
             "build/qemu_harness --disk build/tracer_secondary.img --disk2 %s --name samir_ovl_%s --out build "
             "--timeout-ms 60000 --mouse '%s' --keys-after SHELL-READY --quit-after SHELL-DONE > " D "/%s.stdout 2> " D "/%s.report",
             img, mode,
             ok ? "ko,kv,kl,kret,w3000,kshift-slash,kspc,kapostrophe,ko,kv,kl,kapostrophe,kshift-equal,kapostrophe,kminus,ko,kk,kapostrophe,kret,"
                  "w1000,kq,ku,ki,kt,kret,w2000,ke,kx,ki,kt,kret"
                : "ko,kv,kl,kret,w3000,kret,w3000,kctrl-c,w1000,kn,kret,w1000,kq,ku,ki,kt,kret,w1000,ke,kx,ki,kt,kret",
             mode, mode);
    command(cmd);
    snprintf(cmd,sizeof cmd,D "/%s.report",mode);
    { char report[16384]; FILE *r=fopen(cmd,"rb");size_t z=0;
      if(r){z=fread(report,1,sizeof report-1,r);fclose(r);}report[z]=0;
      fprintf(stderr,"%s",report);
      CHECK(strstr(report,"[harness] OK=1\n") && strstr(report,"quit_after_found=1") &&
            strstr(report,ok?"mouse_events_sent=31 ":"mouse_events_sent=23 "),"overlay refusal entire trace completed");
    }
    snprintf(cmd, sizeof cmd, "build/samir_ovl_%s.serial", mode);
    f = fopen(cmd, "rb");
    CHECK(f != NULL, "serial captured");
    if (!f) return TEST_SUMMARY("samir-ovl-emu");
    n = fread(serial, 1, sizeof serial - 1, f);
    fclose(f);
    serial[n] = 0;
    CHECK(strstr(serial, "SHELL-DONE") != NULL, "guest shell returned (no hang)");
    CHECK(strstr(serial, "PC LOAD LETTER") == NULL, "guest did not panic");
    if (ok) {
        CHECK(count(serial, prompt) == 0, "OK: no Insert System Disk prompt with the right SAMIR.OVL");
        CHECK(strstr(serial, "ovl-ok") != NULL, "OK: the dot prompt ran a command");
        CHECK(strstr(serial, "SAMIR-REFUSED-1") == NULL, "OK: QUIT returned 0");
    } else {
        char what[96];
        snprintf(what, sizeof what, "%s: Insert System Disk prompt shown, then again after ENTER", mode);
        CHECK(count(serial, prompt) >= 2, what);
        snprintf(what, sizeof what, "%s: Ctrl-C aborted with return code 1", mode);
        CHECK(strstr(serial, "SAMIR-REFUSED-1") != NULL, what);
        snprintf(what, sizeof what, "%s: no dot-prompt command ran", mode);
        CHECK(strstr(serial, "ovl-ok") == NULL, what);
    }
    return TEST_SUMMARY("samir-ovl-emu");
}
