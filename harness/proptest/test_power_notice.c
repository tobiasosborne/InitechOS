/* B04 / audit L022: exact authored explanation, app identity, and physical
 * fit in the independently established s8_alert_modal message column.
 * Does not generate its expected words from the implementation. */
#include <stdio.h>
#include <string.h>
#include "power_notice.h"
#include "chicago12.h"
int main(void)
{
    const char *lines[3]={"","",""};
    int fails=0;
    for(int asked=0;asked<2;asked++) {
        unsigned n=finder_power_notice(asked,"CALCULAT.EXE",lines);
        if(n!=3 || strcmp(lines[0],"Shut Down or Restart was cancelled.") ||
           strcmp(lines[1],"CALCULAT.EXE") ||
           strcmp(lines[2],asked?"The application did not quit.":
                                "Close this application and try again.")) {
            puts("FAIL power-notice: cancellation must name the app and reason");fails++;
        }
        for(unsigned i=0;i<n;i++) {
            unsigned w=0;
            for(const char *p=lines[i];*p;p++) w+=(unsigned)chicago12_advance((unsigned char)*p);
            if(w>285 || strlen(lines[i])>=48) {puts("FAIL power-notice: text clipped");fails++;}
        }
    }
    printf("test_power_notice: %d failures\n",fails);
    return fails?1:0;
}
