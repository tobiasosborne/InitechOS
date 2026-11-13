/* Round 8: a dead native section must not root an LTO callee permanently.
 * Ref: CODEX-OUT.md, LTO section probe; round-2 policy Sec 4 lever C.
 * The linker discards this function; its relocation still reaches the LTO
 * plugin before section GC. Factory fixture only, never executed. */
extern unsigned gc_dead_function(void);

unsigned gc_native_dead(void)
{
    return gc_dead_function();
}
