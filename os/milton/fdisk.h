/* Primary-partition editor: IBM DOS TR pp9-6..9; MS-DOS UG pp287..295.
 * DEC-17 service; Appendix A console/process ABI. */
#ifndef INITECH_FDISK_H
#define INITECH_FDISK_H
#include <stdint.h>
typedef struct {
    int (*read)(unsigned drive,uint8_t sector[512]);
    int (*write)(unsigned drive,const uint8_t sector[512]);
    uint32_t (*capacity)(unsigned drive);
    int (*get)(void);
    void (*put)(const char *);
    unsigned loaded_drive;
    void (*restart)(void);
} fdisk_io;
int fdisk_create(uint8_t mbr[512],uint32_t sectors,unsigned cylinders,int active);
int fdisk_active(uint8_t mbr[512],unsigned partition);
int fdisk_delete(uint8_t mbr[512],int confirmed);
int fdisk_run(const fdisk_io *io);
#endif
