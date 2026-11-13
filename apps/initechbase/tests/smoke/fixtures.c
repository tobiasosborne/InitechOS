/* Independent invoice fixture: tests/test_canon_y2k.c's documented schema/data,
 * encoded using spec/ibase/dbf_format.h's on-disk format (no codec linked).
 * PARTS is minted by prog_diff's existing independent byte writer. */
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
    unsigned char h[193] = {3,93,7,19,4,0,0,0,193,0,35,0};
    const char *names[] = {"INVNO","CUST","AMOUNT","DUEDATE","PAID"};
    const char types[] = "CCNDL";
    const unsigned widths[] = {5,10,10,8,1};
    const char *rows[] = {
        " A1001INITROingp   1250.0019991215F",
        " A1002ACME CORP     875.5019000105F",
        " A1003GLOBEX INC   4400.0019991130T",
        " A1004SOYLENT CO     99.9919000210F"
    };
    FILE *f;
    if (argc != 2 || !(f=fopen(argv[1],"wb"))) return 1;
    for (unsigned i=0; i<5; i++) {
        unsigned char *d=h+32+32*i;
        memcpy(d,names[i],strlen(names[i])); d[11]=types[i];
        d[16]=(unsigned char)widths[i]; d[17]=i==2?2:0;
    }
    h[192]=13;
    if (fwrite(h,1,sizeof h,f)!=sizeof h) return 1;
    /* Fixed-width numeric columns have ten bytes, including leading blanks. */
    for (unsigned i=0; i<4; i++) {
        if (strlen(rows[i])!=35 || fwrite(rows[i],1,35,f)!=35) return 1;
    }
    if (fputc(26,f)==EOF) return 1;
    return fclose(f)!=0;
}
