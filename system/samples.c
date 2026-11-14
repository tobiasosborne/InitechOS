/* Initech-authored examples, not vendor data. DBF III byte layout:
 * apps/initechbase/spec/ibase/dbf.md sections 2 and 4. Sample identities:
 * tests/mint_clients_dbf.c, xbase_prog_diff/prog_diff.c, smoke/fixtures.c.
 * No application codec or host clock is involved. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct field { const char *name; unsigned char type, width, decimals; };
static void table(const char *dir, const char *name, const struct field *fields,
                  unsigned nf, const char *const *rows, unsigned nr)
{
    unsigned char header[32] = {3,93,7,19};
    unsigned hl = 33 + nf * 32, rl = 1;
    char path[4096];
    FILE *f;
    for (unsigned i = 0; i < nf; i++) rl += fields[i].width;
    header[4] = (unsigned char)nr;
    header[8] = (unsigned char)hl; header[9] = (unsigned char)(hl >> 8);
    header[10] = (unsigned char)rl; header[11] = (unsigned char)(rl >> 8);
    if (snprintf(path, sizeof path, "%s/%s", dir, name) >= (int)sizeof path ||
        !(f = fopen(path, "wb"))) { perror(name); exit(1); }
    if (fwrite(header, 1, 32, f) != 32) exit(1);
    for (unsigned i = 0; i < nf; i++) {
        unsigned char field[32] = {0};
        memcpy(field, fields[i].name, strlen(fields[i].name));
        field[11] = fields[i].type; field[16] = fields[i].width;
        field[17] = fields[i].decimals;
        if (fwrite(field, 1, 32, f) != 32) exit(1);
    }
    if (fputc(13, f) == EOF) exit(1);
    for (unsigned i = 0; i < nr; i++) {
        if (strlen(rows[i]) != rl) {
            fprintf(stderr, "%s: invalid record width\n", name); exit(1);
        }
        if (fwrite(rows[i], 1, rl, f) != rl) exit(1);
    }
    if (fputc(26, f) == EOF || fclose(f)) exit(1);
}
int main(int argc, char **argv)
{
    const struct field clients[] = {{"NAME",'C',10,0},{"CITY",'C',12,0},{"BAL",'N',8,2}};
    const char *const client_rows[] = {
        " PESTON    HONOLULU     1234.50",
        " WADDAMS   AUSTIN        -42.00",
        " LUMBERGH  DALLAS       7777.77"
    };
    const struct field parts[] = {{"CODE",'C',3,0},{"QTY",'N',5,0},{"OK",'L',1,0}};
    const char *const part_rows[] = {" AAA  100T"," BBB  200F"," CCC  100T"," DDD  300T"};
    const struct field invoice[] = {
        {"INVNO",'C',5,0},{"CUST",'C',10,0},{"AMOUNT",'N',10,2},
        {"DUEDATE",'D',8,0},{"PAID",'L',1,0}
    };
    const char *const invoice_rows[] = {
        " A1001INITECH      1250.0019991215F",
        " A1002ACME CORP     875.5019000105F",
        " A1003GLOBEX INC   4400.0019991130T",
        " A1004SOYLENT CO     99.9919000210F"
    };
    if (argc != 2) return 1;
    table(argv[1], "CLIENTS.DBF", clients, 3, client_rows, 3);
    table(argv[1], "PARTS.DBF", parts, 3, part_rows, 4);
    table(argv[1], "INVOICE.DBF", invoice, 5, invoice_rows, 4);
    return 0;
}
