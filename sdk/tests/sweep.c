/* The published dead-command sweep (sdk/host/command_sweep.h) over a toy
 * tenant: run with SWEEP_TOY_DEAD 0 it must pass, with 1 it must fail on
 * the dead Beep row (tests/run.sh grades both). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbx_host.h"
#include "sweep/toy.c"
#define SWEEP_BLOCK "SWEEPTOY.EXE"
#define SWEEP_NAME "sdk_sweep_toy"
#include "command_sweep.h"
static void sweep_launch(void)
{
    tbx_host_reset();
    if (tbx_app_main() != 0) { printf("FAIL " SWEEP_NAME ": launch\n"); exit(2); }
    sw_event(updateEvt, 0, 0, 0, 0);
}
int main(void) { return command_sweep_main(); }
