/* Factory adapter: run the shipped utility core over the real FAT backend.
 * Independent expected results come from mattrib/mlabel/mdir on cloned images,
 * never from this adapter. Ref: PRD 6.1; fat12.h directory/CHMOD interfaces. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dosutil.h"
#include "fat12.h"
#include "blockdev_file.h"
static blockdev_file_t disk;
static fat12_volume_t vol;
static uint8_t fat[65536], scratch[65536];
static uint32_t fatlen;
static const char *input;
static unsigned wanted;
static du_entry *found;
static int visit(const dir_entry_t *e, void *ctx)
{
    (void)ctx;
    if (e->attribute & 8) return 0;
    if (wanted--) return 0;
    fat12_format_83(e, found->name);
    found->attr=e->attribute;
    return 1;
}
int du_list(const char *path, unsigned index, du_entry *out)
{
    fat12_dir_t parent, dir;
    dir_entry_t e;
    int rc=fat12_resolve_path(&vol,scratch,fat,fatlen,path,&parent,&e);
    if (rc<0 || !(e.attribute & 16)) return -1;
    dir.is_root=e.start_cluster==0;
    dir.start_cluster=e.start_cluster;
    wanted=index; found=out;
    rc=fat12_read_dir(&vol,&dir,scratch,fat,fatlen,visit,NULL);
    return rc<0 ? -1 : rc==1;
}
int du_attr(const char *path, int set, uint8_t *a)
{
    fat12_dir_t parent;
    dir_entry_t e;
    int rc=fat12_resolve_path(&vol,scratch,fat,fatlen,path,&parent,&e);
    if (rc<0) return -1;
    const char *leaf=strrchr(path,'\\');
    leaf=leaf ? leaf+1 : path;
    if (set) return fat12_set_attr(&vol,fat,fatlen,leaf,
        parent.is_root ? 0 : parent.start_cluster,*a,scratch);
    *a=e.attribute;
    return 0;
}
int du_cwd(unsigned drive, char out[64])
{
    const char *cwd=getenv("DOSUTIL_CWD");
    if (drive) return -1;   /* the host oracle mounts A: only */
    if (!cwd) cwd="";
    if (strlen(cwd)>63) return -1;
    strcpy(out,cwd); return 0;
}
int du_drive(void) { return 0; }
int du_sector(int write, unsigned drive, unsigned lba, uint8_t buf[512])
{
    if (drive) return -1;
    return write ? disk.dev.write_sectors(disk.dev.ctx,lba,1,buf)
                 : disk.dev.read_sectors(disk.dev.ctx,lba,1,buf);
}
int du_line(char out[128])
{
    int n=0;
    while (*input && *input!='\n' && *input!='\r') {
        if (n<127) out[n++]=*input;
        input++;
    }
    if (*input=='\r') input++;
    if (*input=='\n') input++;
    out[n]=0; return n;
}
void du_puts(const char *s) { fputs(s,stdout); }
int main(int argc, char **argv)
{
    if (argc<4 || blockdev_file_open_rw(&disk,argv[1])) return 2;
    if (fat12_mount(&vol,&disk.dev,scratch) ||
        fat12_read_fat(&vol,fat,sizeof(fat))) return 2;
    fatlen=vol.bpb.sectors_per_fat*512u;
    input=argc>4 ? argv[4] : "";
    int rc=!strcmp(argv[2],"attrib") ? attrib_main(argv[3]) : label_main(argv[3]);
    blockdev_file_close(&disk);
    return rc;
}
