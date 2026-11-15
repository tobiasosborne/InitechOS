/* GOLDEN-MASTER item 4 dead-command sweep over the real tenant and its
 * dialog item lists. The inventory is the README's; see command_sweep.h. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbx_host.h"
#include "../cpanels.c"
#define SWEEP_BLOCK "CPANELS.EXE"
#define SWEEP_NAME "test_cpanels_commands"
#include "command_sweep.h"
static void sweep_launch(void)
{
    tbx_host_reset();
    if (tbx_app_main() != 0) { printf("FAIL " SWEEP_NAME ": launch\n"); exit(2); }
    sw_event(updateEvt, 0, 0, 0, 0);
}
int main(void) { return command_sweep_main(); }
