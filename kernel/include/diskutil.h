/* Ref: ADR-0003 DEC-08/17/18; MS-DOS UG pp59-63,74-76,119. */
#ifndef INITECH_DISKUTIL_H
#define INITECH_DISKUTIL_H
#include <stdint.h>
typedef struct { uint32_t disk, partition, base, sectors, flags; } du_map;
#define DU_RUNNING 1u
#define DU_REMOVABLE 2u
typedef struct {
 int (*map)(unsigned,du_map *);
 int (*sector)(unsigned,uint32_t,void *,int);
 int (*get)(void);
 void (*put)(const char *);
} du_io;
/* kind: 0 FORMAT, 1 SYS, 2 DISKCOPY, 3 DISKCOMP. */
int diskutil_run(const du_io *,unsigned,const char *);
#endif
