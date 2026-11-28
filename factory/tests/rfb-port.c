#include "../emu/rfb_port.h"
static int check(const char *text,int expected)
{
    FILE *f=tmpfile();if(!f)return 1;
    fputs(text,f);rewind(f);int actual=bochs_rfb_log_port(f);fclose(f);
    if(actual==expected)return 0;
    fprintf(stderr,"FAIL RFB port: expected %d actual %d\n",expected,actual);return 1;
}
int main(void)
{
    if(check("",0) || check("[RFB   ] Trying port 5900\n",0) ||
       check("[RFB   ] Trying port 5900\n[RFB   ] Could not bind socket\n[RFB   ] Trying port 5901\n",0) ||
       check("000000i[RFB   ] listening for connections on port 5901\n",0) ||
       check("000000i[RFB   ] Trying port 5900\n"
             "[initech-rfb] bound loopback port 38417\n",38417) ||
       check("[OTHER ] listening for connections on port 5900\n",0) ||
       check("[initech-rfb] bound loopback port 65536\n",0) ||
       check("[initech-rfb] bound loopback port 38417",0) ||
       check("[initech-rfb] bound loopback port 0\n",0))return 1;
    puts("ok RFB port: adapter publication accepted; Bochs guesses, partial and invalid lines rejected");return 0;
}
