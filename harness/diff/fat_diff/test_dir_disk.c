/* Exercise the real FAT backend vs mtools mdir, independently of the shell.
 * Ref: DOS 3.30 Technical Reference pp.5-5, 5-11..13 / PDF 112,118..120;
 * free clusters are zero entries, times/dates are the directory packed words.
 * mdir supplies the emulator cell golden too (no command.c formatter linked). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blockdev_file.h"
#include "fileio_fat.h"
#include "int21.h"
static const int21_file_backend_t *backend;
void int21_set_file_backend(const int21_file_backend_t *b) { backend = b; }
void int21_set_media(uint8_t media) { (void)media; }   /* AH=1Bh/1Ch seam (509.9) */
/* This read-only oracle binds no clock. The integration stamp oracle binds
 * the real INT21 seam instead (dos_parity_host.c). */
int int21_file_stamp(uint16_t *t, uint16_t *d) { (void)t; (void)d; return 0; }

/* Fixed on-disk fixtures, independent of the artifact label reader. They
 * deliberately disagree between root and BPB, to prove root precedence. */
static int label_fixture(const char *src, const char *dst, const char *kind)
{
    FILE *a=fopen(src,"rb"), *b=fopen(dst,"wb");
    unsigned char buf[512], boot[512];
    size_t n;
    if (!a || !b) return 2;
    while ((n=fread(buf,1,sizeof buf,a))) if (fwrite(buf,1,n,b)!=n) return 2;
    fclose(a); fclose(b);
    if (!(b=fopen(dst,"r+b")) || fread(boot,1,512,b)!=512) return 2;
    unsigned root=(boot[14]|boot[15]<<8)+boot[16]*(boot[22]|boot[23]<<8);
    /* Reserve the first root slot for a synthetic label; all other entries
     * remain unchanged. These tests only grade label identity. */
    memset(buf,0,32);
    memcpy(buf,"ROOT LABEL ",11); buf[11]=8;
    if (strcmp(kind,"root")) buf[0]=0xE5;
    const char *bl=!strcmp(kind,"none")?"NO NAME    ":"BOOT LABEL ";
    memcpy(boot+43,bl,11); boot[38]=0x29;
    rewind(b); if (fwrite(boot,1,512,b)!=512) return 2;
    if (fseek(b,root*512u,SEEK_SET) || fwrite(buf,1,32,b)!=32) return 2;
    fclose(b); return 0;
}

int main(int argc, char **argv)
{
    blockdev_file_t bf;
    fat12_volume_t vol;
    unsigned char sec[512];
    uint16_t spc, bps, total, freec;
    char line[256], name[16], ext[16], size[16], date[32], time[32];
    unsigned long ref_free = 0;
    int have_free = 0, have_volume = 0, count = 0, row = 7;
    FILE *f, *cells;
    if (argc==5 && !strcmp(argv[1],"--fixture"))
        return label_fixture(argv[2],argv[3],argv[4]);
    if (argc != 4) return 2;
    int label_only=!strcmp(argv[1],"--label");
    const char *image=label_only?argv[2]:argv[1];
    if (blockdev_file_open(&bf, image) || fat12_mount(&vol, &bf.dev, sec) ||
        fileio_fat_bind(&vol) || backend->freespace(&spc,&bps,&total,&freec)) return 2;
    if (label_only) {
        char label[12];
        if (fileio_fat_volume_label(label) || strcmp(label,argv[3])) {
            fprintf(stderr,"DIR LABEL RED: wanted [%s] got [%s]\n",argv[3],label);
            return 1;
        }
        blockdev_file_arm_read_fault(&bf,1);
        if (fileio_fat_volume_label(label)>=0) {
            fprintf(stderr,"DIR LABEL RED: read error silently fell back\n"); return 1;
        }
        printf("DIR LABEL PASS: [%s], root/BPB selection, read faults loud\n",argv[3]);
        blockdev_file_close(&bf); return 0;
    }
    if (!(f = fopen(argv[2], "r")) || !(cells = fopen(argv[3], "w"))) return 2;
    /* Shell starts with CLS, then echoes A:\>dir on row 0. Manual headers
     * occupy rows 2 and 4; rows 1,3,5 are explicitly blank. */
    fprintf(cells, "3 0 %80s\n4 0  Directory of A:\\\n5 0 %80s\n", "", "");
    row = 6;
    while (fgets(line, sizeof line, f)) {
        char digits[32]; int k = 0;
        if (strstr(line,"Volume in drive")) {
            char *label=strstr(line," is ");
            if (label) {
                label+=4;
                size_t len=strlen(label);
                while (len && (label[len-1]==' ' || label[len-1]=='\n' || label[len-1]=='\r')) label[--len]=0;
                fprintf(cells,"2 0  Volume in drive A is %s\n",label);
            } else if (strstr(line,"has no label")) {
                fprintf(cells,"2 0  Volume in drive A has no label\n");
            } else return 2;
            have_volume=1;
        } else if (strstr(line,"bytes free")) {
            for (char *s=line; *s && s < strstr(line,"bytes free"); s++)
                if (*s >= '0' && *s <= '9') digits[k++] = *s;
            digits[k] = 0; ref_free = strtoul(digits, NULL, 10); have_free = 1;
        } else if (strstr(line,"<DIR>")) {
            int y,m,d,hh,mm;
            if (sscanf(line,"%15s <DIR> %d-%d-%d %d:%d",name,&y,&m,&d,&hh,&mm)!=6) return 2;
            fprintf(cells,"%d 0 %-8s     %8s  %2d-%02d-%02d  %2d:%02d%c\n",
                    row++,name,"<DIR>",m,d,y%100,hh%12?hh%12:12,mm,hh>=12?'p':'a');
            count++;
        } else if (sscanf(line,"%15s %15s %15s %31s %31s",name,ext,size,date,time)==5 &&
                   strchr(date,'-') && strchr(time,':')) {
            int y,m,d,hh,mm;
            if (sscanf(date,"%d-%d-%d",&y,&m,&d)!=3 || sscanf(time,"%d:%d",&hh,&mm)!=2) return 2;
            /* mdir uses 24-hour ISO fields; the DOS display is 12-hour a/p. */
            fprintf(cells,"%d 0 %-8s %-3s %8s  %2d-%02d-%02d  %2d:%02d%c\n",
                    row++, name, ext, size, m,d,y%100, hh%12 ? hh%12 : 12,mm,hh>=12?'p':'a');
            count++;
        }
    }
    fprintf(cells,"%d 0 %9d File(s) %9lu bytes free\n", row+1,count,ref_free);
    fclose(f); fclose(cells); blockdev_file_close(&bf);
    if (!have_volume || !have_free || (unsigned long)spc*bps*freec != ref_free) {
        fprintf(stderr,"DIR SPACE RED: FAT=%lu mdir=%lu\n",(unsigned long)spc*bps*freec,ref_free);
        return 1;
    }
    printf("DIR SPACE PASS: FAT=%lu == mdir=%lu bytes free (%u clusters x %u bytes)\n",
           ref_free,ref_free,freec,spc*bps);
    return 0;
}
