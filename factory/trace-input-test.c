/* RFC 6143/X11 transport values, independently literal expectations. */
#include "emu/bochs_trace.h"
int main(void)
{
    BochsTraceStep s;
    if (!bochs_trace_parse("kslash",6,&s) || s.kind!=BX_KEY || s.key!='/') return 1;
    if (!bochs_trace_parse("kshift-2",8,&s) || s.key!='2' || !s.shift) return 1;
    if (!bochs_trace_parse("kctrl-q",7,&s) || s.key!='q' || !s.ctrl) return 1;
    if (bochs_trace_parse("kunknown",8,&s)) return 1;
    return 0;
}
