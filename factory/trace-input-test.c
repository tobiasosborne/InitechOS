/* RFC 6143/X11 transport values, independently literal expectations. */
#include "emu/bochs_trace.h"
int main(void)
{
    BochsTraceStep s;
    if (!bochs_trace_parse("kslash",6,&s) || s.kind!=BX_KEY || s.key!='/') return 1;
    if (!bochs_trace_parse("kshift-2",8,&s) || s.key!='2' || !s.shift) return 1;
    if (!bochs_trace_parse("kctrl-q",7,&s) || s.key!='q' || !s.ctrl) return 1;
    if (bochs_trace_parse("kunknown",8,&s)) return 1;
    /* T<name>: a text-page capture into <out>/<name>.screen (stage 2). */
    if (!bochs_trace_parse("Tver_1-b",8,&s) || s.kind!=BX_TEXT || strcmp(s.name,"ver_1-b")) return 1;
    if (bochs_trace_parse("T",1,&s) || bochs_trace_parse("Ta/b",4,&s) || bochs_trace_parse("Ta.b",4,&s)) return 1;
    if (bochs_trace_parse("T0123456789012345678901234567890123456789",41,&s)) return 1;
    /* S<name>: a lossless Bochs snapshot into <out>/<name>.ppm (F2). */
    if (!bochs_trace_parse("Sfinder-info",12,&s) || s.kind!=BX_SNAP || strcmp(s.name,"finder-info")) return 1;
    if (bochs_trace_parse("S",1,&s) || bochs_trace_parse("Sa/b",4,&s)) return 1;
    return 0;
}
