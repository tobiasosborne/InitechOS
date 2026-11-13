/* Factory-only fault injection: a real close followed by an I/O error.
 * The relocation stream is the first stream closed when --reloc-file is used. */
#include <stdio.h>
#include <errno.h>
int __real_fclose(FILE *);
int __wrap_fclose(FILE *f)
{
    int rc=__real_fclose(f);
    static int first=1;
    if(first){first=0;errno=EIO;return EOF;}
    return rc;
}
