/* Independent deadline contract: late evidence cannot undo an expired run.
 * Test the same final predicate used by both frontends, without a timing race. */
#include "../emu/bochs.c"
int main(void)
{
    BochsConfig cfg={.keys_script="ret",.expect_marker="ready",.quit_after="done"};
    BochsResult out={.launched=true,.rfb_unblocked=true,.marker_found=true,
        .quit_marker_found=true,.term_signal=SIGTERM};
    if(!bochs_verdict(&cfg,&out,true))return 1;
    out.timed_out=true;
    if(bochs_verdict(&cfg,&out,true)) {
        fputs("FAIL Bochs: timeout accepted with late final completion marker\n",stderr);return 1;
    }
    out.timed_out=false;
    if(bochs_verdict(&cfg,&out,false))return 1;
    out.triple_fault=true;
    if(bochs_verdict(&cfg,&out,true))return 1;
    puts("ok Bochs: timely SIGTERM accepted; timeout, incomplete input and fault rejected");
    return 0;
}
