/* Kernel-only section-GC oracle, initech-tode.
 * Ref: docs/design/kernel-size-policy-round2.md Sec 4 lever C;
 * PRD Sec 5 (flat kernel), CLAUDE.md Rules 6/11.
 * A live relocation chain must survive; unrelated code/data must disappear.
 * This fixture is factory input, never shipped or run as a DOS program.
 */
volatile unsigned gc_live_data = 0x12345678u;
volatile unsigned gc_dead_data = 0x87654321u;

__attribute__((noinline)) unsigned gc_live_function(void)
{
    return gc_live_data;
}

__attribute__((noinline)) unsigned gc_dead_function(void)
{
    return gc_dead_data;
}

void _start(void)
{
    gc_live_data = gc_live_function();
    for (;;) __asm__ __volatile__("hlt");
}
