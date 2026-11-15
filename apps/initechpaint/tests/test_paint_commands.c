/* GOLDEN-MASTER item 4 dead-command sweep over the real tenant. The
 * inventory is the README's; see command_sweep.h for what is checked. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbx_host.h"
#include "../paint.c"
#define SWEEP_BLOCK "PAINT.EXE"
#define SWEEP_NAME "test_paint_commands"
#include "command_sweep.h"
static void sweep_launch(void)
{
    tbx_host_reset();
    if (tbx_app_main() != 0) { printf("FAIL " SWEEP_NAME ": launch\n"); exit(2); }
    sw_event(updateEvt, 0, 0, 0, 0);
}
int main(void) { return command_sweep_main(); }
