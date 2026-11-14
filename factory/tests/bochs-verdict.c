/* Independent process/deadline contract, using the actual final predicate. */
#include "../emu/bochs.c"
#define REQUIRE(cond, message) do { if (!(cond)) { puts("FAIL Bochs: " message); return 1; } } while (0)
int main(void)
{
    BochsConfig cfg={.keys_script="ret",.expect_marker="ready",.quit_after="done"};
    BochsResult out={.launched=true,.rfb_unblocked=true,.marker_found=true,
        .quit_marker_found=true,.reaped=true,.shutdown_requested=true,
        .exit_code=-1,.term_signal=SIGTERM};
    REQUIRE(bochs_verdict(&cfg,&out,true), "intentional SIGTERM rejected");
    out.shutdown_requested=false;
    REQUIRE(!bochs_verdict(&cfg,&out,true), "spontaneous SIGTERM accepted");
    out.term_signal=0; out.exit_code=3;
    REQUIRE(!bochs_verdict(&cfg,&out,true), "abnormal exit accepted");
    out.exit_code=0;
    REQUIRE(bochs_verdict(&cfg,&out,true), "clean normal exit rejected");
    out.exit_code=-1;
    for (int requested=0;requested<2;requested++) {
        out.shutdown_requested=requested;
        int signals[]={SIGSEGV,SIGKILL,SIGABRT};
        for(unsigned i=0;i<sizeof signals/sizeof signals[0];i++) {
            out.term_signal=signals[i];
            REQUIRE(!bochs_verdict(&cfg,&out,true), "crash/escalation accepted");
        }
    }
    out.term_signal=SIGTERM; out.shutdown_requested=true;
    out.reaped=false;
    REQUIRE(!bochs_verdict(&cfg,&out,true), "missing reap accepted");
    out.reaped=true; out.timed_out=true;
    REQUIRE(!bochs_verdict(&cfg,&out,true), "timeout accepted with late final completion marker");
    out.timed_out=false;
    REQUIRE(!bochs_verdict(&cfg,&out,false), "incomplete input accepted");
    out.triple_fault=true;
    REQUIRE(!bochs_verdict(&cfg,&out,true), "CPU fault accepted");
    puts("ok Bochs: clean exit and timely intentional SIGTERM accepted; abnormal exit, spontaneous signals, escalation, missing reap, timeout, incomplete input and fault rejected");
    return 0;
}
