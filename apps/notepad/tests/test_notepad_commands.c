/* GOLDEN-MASTER item 4 dead-command sweep over the real tenant, compiled
 * twice (Note Pad, then SCRAPBOOK). The inventory is the README's; see
 * command_sweep.h for what is checked. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../notepad.c"
#include "tbx_host.h"
#ifdef SCRAPBOOK
#define SWEEP_BLOCK "SCRAPBK.EXE"
#define SWEEP_NAME "test_scrapbook_commands"
#else
#define SWEEP_BLOCK "NOTEPAD.EXE"
#define SWEEP_NAME "test_notepad_commands"
#endif
#include "command_sweep.h"
static void sweep_launch(void)
{
    tbx_host_reset(); tbx_host.origin_h = 166; tbx_host.origin_v = 102; tbx_host.ch = 237;
    if (tbx_app_main() != 0) { printf("FAIL " SWEEP_NAME ": launch\n"); exit(2); }
    sw_event(updateEvt, 0, 0, 0, 0);
}
int main(void) { return command_sweep_main(); }
