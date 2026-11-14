/* Deterministic test inputs, independent of InitechOS. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){if(argc!=4)return 2;FILE*f=fopen(argv[1],"wb");if(!f)return 2;long n=strtol(argv[2],NULL,0);const char*s=argv[3];size_t l=strlen(s);for(long i=0;i<n;i++)if(fputc(s[i%(long)l],f)==EOF)return 2;return fclose(f)!=0;}
