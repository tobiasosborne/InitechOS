/* GOLDEN-MASTER item 4 dead-command sweep over the real tenant. The
 * inventory is the README's; see command_sweep.h for what is checked. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbx_host.h"
#include "../itext.c"
#define SWEEP_BLOCK "TEXT.EXE"
#define SWEEP_NAME "test_itext_commands"
#include "command_sweep.h"
static void sweep_launch(void)
{
    tbx_host_reset();
    /* A fresh launch's BSS, as test_itext_route's launch() restores it. */
    memset(&g_te, 0, sizeof g_te);
    g_win = 0; g_dirty = 0; g_active = 1; g_scroll = 0; g_anchor = 0;
    g_scrap_has = 0; g_after = 0; g_sf_kind = 0; g_path[0] = 0;
    g_undo_ok = 0; g_undo_run = 0;
    if (tbx_app_main() != 0) { printf("FAIL " SWEEP_NAME ": launch\n"); exit(2); }
    sw_event(updateEvt, 0, 0, 0, 0);
}
int main(void) { return command_sweep_main(); }
