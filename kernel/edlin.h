/* Ref: ADR-0003 Appendix A and DEC-08; MS-DOS 3.3 User's Guide pp.162-193. */
#ifndef MILTON_EDLIN_H
#define MILTON_EDLIN_H
typedef struct {
    int (*get)(void);                 /* AH=07h raw byte, 00 then scan code */
    int (*open)(const char *);        /* AH=3Dh; negative DOS error */
    int (*create)(const char *);      /* AH=3Ch */
    int (*read)(int, void *, unsigned);
    int (*write)(int, const void *, unsigned);
    int (*close)(int);
    int (*remove)(const char *);
    int (*rename)(const char *, const char *);
    int (*attr)(const char *);        /* AH=43h AL=00 */
} edlin_io;
int edlin_run(const edlin_io *, const char *);
#endif
