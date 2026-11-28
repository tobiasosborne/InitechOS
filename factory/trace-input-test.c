/* RFC 6143/X11 transport values, independently literal expectations. */
#include "emu/bochs_trace.h"
#include <stdio.h>
int main(void)
{
    BochsTraceStep s;
    if (!bochs_trace_parse("kslash",6,&s) || s.kind!=BX_KEY || s.key!='/') return 1;
    if (!bochs_trace_parse("kshift-2",8,&s) || s.key!='2' || !s.shift) return 1;
    if (!bochs_trace_parse("kctrl-q",7,&s) || s.key!='q' || !s.ctrl) return 1;
    /* x11-pc-us.map and X11/keysymdef.h: send the base key with its
     * modifier. keys.c emits bare tokens; desktop journeys prefix k. */
    static const struct { const char *token; unsigned key; int shift; } punctuation[] = {
        {"shift-semicolon", 0x003b, 1}, {"kshift-semicolon", 0x003b, 1},
        {"backslash", 0x005c, 0}, {"kbackslash", 0x005c, 0},
        {"minus", 0x002d, 0}, {"kminus", 0x002d, 0}
    };
    for (unsigned i = 0; i < sizeof punctuation / sizeof punctuation[0]; i++) {
        const char *token = punctuation[i].token;
        if (!bochs_trace_parse(token, strlen(token), &s) || s.kind != BX_KEY ||
            s.key != punctuation[i].key || s.shift != punctuation[i].shift || s.ctrl || s.alt) {
            fprintf(stderr, "FAIL Bochs punctuation token %s\n", token);
            return 1;
        }
    }
    if (!bochs_trace_parse("esc", 3, &s) || s.kind != BX_KEY || s.key != 0xff1b ||
        !bochs_trace_parse("kesc", 4, &s) || s.kind != BX_KEY || s.key != 0xff1b) {
        fputs("FAIL Bochs Escape token\n", stderr);
        return 1;
    }
    if (bochs_trace_parse("kunknown",8,&s)) return 1;
    /* T<name>: a text-page capture into <out>/<name>.screen (stage 2). */
    if (!bochs_trace_parse("Tver_1-b",8,&s) || s.kind!=BX_TEXT || strcmp(s.name,"ver_1-b")) return 1;
    if (bochs_trace_parse("T",1,&s) || bochs_trace_parse("Ta/b",4,&s) || bochs_trace_parse("Ta.b",4,&s)) return 1;
    if (bochs_trace_parse("T0123456789012345678901234567890123456789",41,&s)) return 1;
    return 0;
}
