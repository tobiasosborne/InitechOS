/* InitechDOS production entry. SYSINIT mounts disks and applies CONFIG.SYS;
 * COMMAND.COM runs AUTOEXEC.BAT and remains the permanent command processor.
 * Reference: legacy-final kernel_shell, IBM DOS 3.30 startup sequence. */
#include "startup.h"
#include "sysinit.h"
#include "../dos/command/entry.h"

void kernel_main(void)
{
    startup();
    if (!sysinit_shell_ready()) {
        for (;;) __asm__ __volatile__("hlt");
    }
    extern void serial_puts(const char *);
    serial_puts("SHELL-READY\n");
    command_repl(1);
    for (;;) __asm__ __volatile__("hlt");
}
