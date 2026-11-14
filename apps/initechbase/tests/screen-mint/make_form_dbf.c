/* Independent literal FORM.DBF/FORM.DBT fixture writer for the forms
 * captures (design-base-forms section 3). No InitechBase code: the byte
 * layout is the documented dBASE III PLUS format (dbase3-decomp
 * specs/file-formats/dbf.md, dbt.md). Real III+ opening these files at
 * mint time is the validation. Usage: make_form_dbf OUTDIR
 * Writes FORM.DBF/.DBT (2 records) and FORM2.DBF/.DBT (1 record).
 * Schema: FC C(8), FN N(10,2), FD D(8), FL L(1), FM M(10). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put32(FILE *f, unsigned v) {
    fputc((int)(v & 255), f); fputc((int)((v >> 8) & 255), f);
    fputc((int)((v >> 16) & 255), f); fputc((int)((v >> 24) & 255), f);
}
static void put16(FILE *f, unsigned v) {
    fputc((int)(v & 255), f); fputc((int)((v >> 8) & 255), f);
}
static void field(FILE *f, const char *name, int type, int len, int dec) {
    char b[32]; memset(b, 0, sizeof b);
    memcpy(b, name, strlen(name));
    b[11] = (char)type; b[16] = (char)len; b[17] = (char)dec;
    fwrite(b, 1, 32, f);
}
static void dbf(const char *path, int nrec, const char *recs) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fputc(0x83, f); fputc(92, f); fputc(3, f); fputc(2, f);
    put32(f, (unsigned)nrec); put16(f, 193); put16(f, 38);
    for (int i = 0; i < 20; i++) fputc(0, f);
    field(f, "FC", 'C', 8, 0); field(f, "FN", 'N', 10, 2);
    field(f, "FD", 'D', 8, 0); field(f, "FL", 'L', 1, 0);
    field(f, "FM", 'M', 10, 0);
    fputc(0x0d, f);
    fwrite(recs, 1, (size_t)(38 * nrec), f);
    fputc(0x1a, f); fclose(f);
}
static void dbt(const char *path, int nblocks, const char *m1, const char *m2) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    char blk[512]; memset(blk, 0, sizeof blk);
    blk[0] = (char)(nblocks + 1);
    fwrite(blk, 1, 512, f);
    memset(blk, 0, sizeof blk); memcpy(blk, m1, strlen(m1));
    blk[strlen(m1)] = 0x1a; blk[strlen(m1) + 1] = 0x1a;
    fwrite(blk, 1, 512, f);
    if (m2) {
        memset(blk, 0, sizeof blk); memcpy(blk, m2, strlen(m2));
        blk[strlen(m2)] = 0x1a; blk[strlen(m2) + 1] = 0x1a;
        fwrite(blk, 1, 512, f);
    }
    fclose(f);
}
int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "usage: make_form_dbf OUTDIR\n"); return 2; }
    char p[1024];
    /* rec: del FC(8) FN(10) FD(8) FL(1) FM(10) = 38 */
    static const char form_recs[] =
        " " "ABCD    " "     12.50" "        " "F" "         1"
        " " "WXYZ    " "     -3.25" "19920115" "T" "         2";
    static const char form2_recs[] =
        " " "ZONE    " "     99.99" "19911231" "T" "         1";
    snprintf(p, sizeof p, "%s/FORM.DBF", argv[1]); dbf(p, 2, form_recs);
    snprintf(p, sizeof p, "%s/FORM.DBT", argv[1]); dbt(p, 2, "memo sentinel", "second memo");
    snprintf(p, sizeof p, "%s/FORM2.DBF", argv[1]); dbf(p, 1, form2_recs);
    snprintf(p, sizeof p, "%s/FORM2.DBT", argv[1]); dbt(p, 1, "other memo", NULL);
    return 0;
}
