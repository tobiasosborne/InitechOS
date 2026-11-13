/* Catalogue storage decoder, not a new diagnostic policy. Source: locked
 * spec/ibase/dbase_msg_codes.tsv (real III+ 1.1 DBASE.MSG), PRD 6.6.
 * Bounded local expansion preserves reentrancy and costs no PAL/BSS reserve.
 */
#ifndef IBASE_MESSAGES_H
#define IBASE_MESSAGES_H
#include "ibase/messages_packed.h"
#define REPL_MSG_COUNT 151

static unsigned ibase_message(unsigned code, char *out, unsigned cap)
{
    const unsigned char *p = ibase_msg_data;
    unsigned char pending[128];
    unsigned n = 0, depth = 0, token;
    if (!cap) return 0;
    out[0] = 0;
    if (code > REPL_MSG_COUNT) return 0;
    while (code--) { while (*p) p++; p++; }
    while (*p || depth) {
        token = depth ? pending[--depth] : *p++;
        if (token >= 128) {
            token -= 128;
            if (token >= IBASE_MSG_PAIRS || depth + 2 > sizeof pending) return 0;
            pending[depth++] = ibase_msg_pairs[token][1];
            pending[depth++] = ibase_msg_pairs[token][0];
        } else {
            if (n + 1 >= cap) return 0;
            out[n++] = (char)token;
        }
    }
    out[n] = 0;
    return n;
}
#endif
