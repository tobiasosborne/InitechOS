/* Factory independent RI reader and artifact dump. No spec constants or
 * packed data used to compute the oracle. initech-uqah, Law 2. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"
static unsigned char ri[131072];
static void ri_string(unsigned long off)
{
    if (off >= sizeof ri) exit(2);
    while (off < sizeof ri && ri[off]) putchar(ri[off++]);
}
int main(int argc, char **argv)
{
    if (argc == 2 && (!strcmp(argv[1],"--resource") || !strcmp(argv[1],"--resource-mutant"))) {
        unsigned char data[32768]; i123_menu_init();
        unsigned n=i123_menu_export(data,sizeof data);
        if(!strcmp(argv[1],"--resource-mutant"))data[12]^=1;
        if(!n||fwrite(data,1,n,stdout)!=n)return 2;
        return 0;
    }
    int loaded=argc==3&&!strcmp(argv[1],"--load");
    if(loaded) {
        FILE *f=fopen(argv[2],"rb");if(!f)return 2;
        unsigned n=(unsigned)fread(ri,1,sizeof ri,f);fclose(f);
        if(i123_menu_load(ri,n)){fputs("menu resource refused\n",stderr);return 1;}
    }
    if (argc == 1 || loaded) {
        if(!loaded)i123_menu_init();
        for (unsigned l = 0; l < i123_menu_count(); l++) {
            const i123_menu_level_t *lv = &i123_menu_levels[l];
            for (unsigned k = 0; k < lv->count; k++) {
                const i123_menu_item_t *it = &i123_menu_items[lv->first+k];
                printf("%s\t%s\t%s\n", i123_menu_paths[l],
                       i123_menu_string(it->name), i123_menu_string(it->desc));
                if (it->child != 65535u) {
                    char child[512];
                    snprintf(child,sizeof child,"%s%s%s",i123_menu_paths[l],l?"/":"",i123_menu_string(it->name));
                    if (it->child >= i123_menu_count() || strcmp(child,i123_menu_paths[it->child])) return 1;
                }
            }
        }
        return 0;
    }
    if (argc != 3) return 2;
    FILE *f = fopen(argv[1],"rb");
    if (!f) { perror("123.RI"); return 2; }
    size_t n = fread(ri,1,sizeof ri,f); fclose(f);
    if (!n || n == sizeof ri) return 2;
    f = fopen(argv[2],"r"); if (!f) return 2;
    char path[512]; unsigned long a,b;
    while (fscanf(f,"%511s\t%lx\t%lx",path,&a,&b) == 3) {
        if (a >= n || b >= n) return 2;
        printf("%s\t",path); ri_string(a); putchar('\t'); ri_string(b); putchar('\n');
    }
    if (!feof(f)) return 2;
    fclose(f); return 0;
}
