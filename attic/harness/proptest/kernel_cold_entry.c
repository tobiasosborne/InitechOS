/* Factory-only cross-TU size oracle. Ref: PRD Sec 5/7; Rule 6/11.
 * The independently specified answer is 7 * 3 + 1 = 22. */
extern unsigned cold_answer(unsigned value);
volatile unsigned cold_result;
void _start(void)
{
    cold_result = cold_answer(7u);
    for (;;) __asm__ __volatile__("hlt");
}
