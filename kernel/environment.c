/* The standalone DOS kernel does not install a graphical environment. */
#include "environment.h"
void __attribute__((weak)) kernel_graphical_environment(void)
{
    static const char message[] = "FLAIR is not installed.\r\n$";
    __asm__ volatile("int $0x21" : : "a"(0x0900u), "d"(message) : "cc", "memory");
}
