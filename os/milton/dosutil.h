/* Flat utility platform seam. Ref: ADR-0003 Appendix A (43h,47h,4Eh/4Fh),
 * DEC-04a flat pointers, DEC-15 INT 25h/26h; spec/find_data.h. */
#ifndef INITECH_DOSUTIL_H
#define INITECH_DOSUTIL_H
#include <stdint.h>
typedef struct du_entry { char name[13]; uint8_t attr; } du_entry;
int du_list(const char *dir, unsigned index, du_entry *out);
int du_attr(const char *path, int set, uint8_t *attr);
int du_cwd(char out[64]);
int du_drive(void);
int du_sector(int write, unsigned drive, unsigned lba, uint8_t buf[512]);
int du_line(char out[128]);
void du_puts(const char *s);
int attrib_main(const char *tail);
int label_main(const char *tail);
#endif
