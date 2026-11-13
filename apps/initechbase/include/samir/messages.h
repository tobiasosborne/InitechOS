/* Catalogue storage decoder, not a new diagnostic policy. Source: locked
 * spec/samir/dbase_msg_codes.tsv (real III+ 1.1 DBASE.MSG), PRD 6.6.
 * Bounded local expansion preserves reentrancy and costs no PAL/BSS reserve.
 */
#ifndef SAMIR_MESSAGES_H
#define SAMIR_MESSAGES_H
#include "samir/messages_packed.h"
#define REPL_MSG_COUNT 151

static unsigned samir_message(unsigned code, char *out, unsigned cap)
{
    const unsigned char *p = samir_msg_data;
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
            if (token >= SAMIR_MSG_PAIRS || depth + 2 > sizeof pending) return 0;
#ifdef SAMIR_SIZE_MUTATE_MSG_PAIR
            if (!token) { if (n + 1 >= cap) return 0; out[n++] = '!'; continue; }
#endif
            pending[depth++] = samir_msg_pairs[token][1];
            pending[depth++] = samir_msg_pairs[token][0];
        } else {
            if (n + 1 >= cap) return 0;
            out[n++] = (char)token;
        }
    }
    out[n] = 0;
    return n;
}
#endif
