# launchkey / initech-rex3

Lane: `lane/launchkey`. Changes remain in the working tree for the PM.
`bd show initech-rex3` returned `no issue found matching "initech-rex3"`.
No beads writes, commits, stashes or branch changes were made.

## Ground truth and implementation

Read CLAUDE.md, PRD 6.1/6.3/6.5/6.6 and 8, ADR-0013 3.2/3.3/7 O-7,
spec/event_model.h, spec/toolbox_gate.h 6/6a, and WL-0165. The actual local
manual is in `../initech-manuals-ref/lotus123/Lotus 1-2-3 2.2 Manuals (1989)/`:
`Lotus 1-2-3 Release 2.2 - Reference Manual.pdf`, printed p. 1-17 / PDF 33.
The rendered page is in build/launchkey/manual/p033.png; OCR of its prefix table
defines backslash as repeating the label's characters to fill the cell.
No golden was fabricated or minted.

DEC-LAUNCH-1 in kmain reserves Ctrl+Alt+backslash for the system. The shared
system_key.h predicate reads EventRecord.modifiers, so the actual event.c
decoder supplies either physical Ctrl/Alt side. Plain backslash follows
the ordinary tenant route. Consuming its duplicate DOS CON byte prevents an
old worksheet prefix from stopping the next launch's discard_through early.
The suspend drain cooks queued input with mask zero, preserving modifier
releases without delivering SAMIR's keys to the GUI. A raw tail=head drop
would leave the launch modifiers latched after QUIT.

## Rule-8 trace changes

SAMIR_SUSP_KEYS changes its launch token from bsl to ctrl-alt-bsl, shared by
the clean O-7 gate and NO_REBUILD mutant. The SAMIR lifetime desktop harness
has two further launcher tokens, also re-keyed (its backslash is a launch
marker). ZS24EXEC_KEYS still uses plain bsl for DOS path separators.

spec/flair_launchkey_traces.mk adds the existing 123 Finder launch geometry,
then backslash-minus-Return in A1; its recovery leg appends the chord,
SAMIR QUIT, a FLAIR-RESUME readiness wait, and Right to B1. The wait is a
deliberate Rule-8 input-order correction: input sent before resume belongs
to SAMIR and is properly drained. No budget or assertion changed.
The oracle requires the entire 72-character
row draw: nine hyphens plus 63 spaces, independently authored from manual
p. 1-17 and the locked nine-character default column width. It also requires
one suspend, successful text exit, one resume, and a post-resume B1 draw.
The recording uses the same trace. Standalone lane gates are not admitted
to the default vector while their clean acceptance remains red.

## Observed gates

All make invocations used -j3; every QEMU invocation had INITECH_QMP_STDIO=1.
No full test vector or Bochs target ran.

Red first, before the modifier requirement:

```
  FAIL harness/proptest/test_flair_routing.c:514: leg(L1): plain backslash and non-launch modifiers reach tenant
test-flair-routing: 129 checks, 5 failures
```

Red first, before cooking suspend-time releases:

```
  FAIL harness/proptest/test_flair_routing.c:521: leg(L5): suspend drain cooks modifier releases before sheet resumes
```

Final host run (build/launchkey-final-host.log):

```
test-flair-routing: 138 checks, 0 failures
>>> test-flair-routing: green
>>> test-flair-routing-mutant: green (all five mutants RED for the named reason)
>>> test-kernel-size-r4: green (flagship keeps >= 16 KiB headroom in the 384-sector window)
>>> test-makefile-vars: green
```

The loaded flagship is 177132 bytes (19476 bytes of headroom). All five
host mutants were rejected on their named lines:

```
  FAIL harness/proptest/test_flair_routing.c:590: leg(A1): a tenant that REGISTERs without a window does NOT become the foreground -- the Finder stays the head (DEC-AC3-4)
  FAIL harness/proptest/test_flair_routing.c:920: leg(T1): the covered title run writes NO pixel inside the front window (chrome_ctext clips to visRgn; bead initech-tdnl.14)
  FAIL harness/proptest/test_flair_routing.c:1065: leg(X2): AX is the LOW WORD of EAX -- REGISTER with EAX=0xBEEF0001 registers (TENANT-GATE ax=0x0001 rec=+0x0000 -> 0)
  FAIL harness/proptest/test_flair_routing.c:1073: leg(X3): with the frame and the stack above the args POISONED, NEWWINDOW reads exactly its five args from frame+68 (TENANT-GATE ax=0x0020 l=340 t=100 r=600 b=320 goaway=1 -> 1)
  FAIL harness/proptest/test_flair_routing.c:518: leg(L1): plain backslash and non-launch modifiers reach tenant
```

The last line is FLAIR_MUT_PLAIN_BSL_LAUNCH. The other knobs are respectively
PROC_MUT_ADMIT_IS_FOREGROUND, CHROME_MUT_TITLE_UNCLIPPED, TBX_MUT_AX_FULL_EAX,
and TBX_MUT_ARG_OFFSET.

The corresponding QEMU mutant also bites (build/launchkey-emu-mutant.log):

```
!!! test-flair-launchkey FAIL: plain backslash did not draw repeat label without SAMIR
>>> test-flair-launchkey-mutant: green (FLAIR_MUT_PLAIN_BSL_LAUNCH RED: plain backslash launched SAMIR)
```

Its serial shows 123 loaded, the cooked backslash, FLAIR-SUSPEND, and SAMIR
trying to interpret the remaining minus as a command.

Existing re-keyed QEMU gates:

```
>>> test-flair-samir-suspend [1/4]: no triple-fault (PRE + POST)
>>> test-flair-samir-suspend [2/4]: FLAIR-SUSPEND + LIST rows (PESTON/WADDAMS/LUMBERGH) + FLAIR-RESUME
>>> test-flair-samir-suspend [3/4]: PRE + POST screendumps captured
>>> test-flair-samir-suspend [4/4]: pre.ppm == post.ppm (desktop rebuilt BIT-IDENTICAL)
>>> test-flair-samir-suspend-mutant: green (no-rebuild mutant correctly RED -- pre != post, no crash)
>>> test-samir-lifetime-desktop: green
>>> test-samir-lifetime-desktop-mutant: green (duplicate USE correctly RED)
```

Lifetime: 241 checks, 0 failures. Its duplicate-USE mutant failed on:

```
  FAIL harness/diff/dbf_diff/test_samir_dbsafe_emu.c:115: L003 real catalog duplicate-open refusal
```

## Worksheet QEMU acceptance

The new test-flair-launchkey gate failed, then failed again alone with the
same timeout and assertions. The standalone rerun delivered backslash and
minus to 123 (TENANT-EVT what=3), but FLAIR-LIVE-OK ended the stock 250-tick
pump before Return committed the label. The first leg therefore failed:

```
!!! test-flair-launchkey FAIL: plain backslash did not draw repeat label without SAMIR
```

Logs for these early runs: build/launchkey-emu.log and
build/launchkey-emu-alone.log. The recovery leg was not reached in those runs.
The gate's budget, timeout and assertions were not loosened.

A later final-source run passed the repeating-label leg and ran SAMIR QUIT
back to FLAIR-RESUME, but failed the post-resume Right assertion. Its serial
is preserved in build/launchkey_resume_before_wait.serial and its log in
build/launchkey-final-emu.log:

```
!!! test-flair-launchkey FAIL: chord / QUIT / sheet recovery
```

After adding the explicit readiness wait, the standalone rerun passed both
legs with the original image, timeout, budget and assertions:

```
>>> test-flair-launchkey: plain backslash-minus draws nine hyphens in A1; no SAMIR launch
>>> test-flair-launchkey: green (Ctrl+Alt+backslash launches SAMIR; QUIT resumes sheet; Right reaches B1)
```

Log: build/launchkey-final-emu-alone.log. The recovery serial has the exact
72-character row draw and the B1 draw after FLAIR-RESUME.

## Rule-14 acceptance still red

test-flair-launchkey-record invokes record-flair SCRIPT=launchkey. It emitted:

```
>>> record-flair [launchkey]: 33 frames captured
>>> record-flair [launchkey]: clips ready
!!! test-flair-launchkey-record FAIL: repeating label / SAMIR / QUIT / resumed sheet
```

The mandatory standalone rerun gave the same failure and the same 33-frame
unaccepted scene (build/launchkey-record-alone.log).

build/clips/launchkey.gif and .mp4 are NOT acceptance clips: the spaced
double-clicks select the volume twice and never open it, so no 123 loads.
The recording still proves the modified chord runs SAMIR and QUIT returns;
it does not prove worksheet behavior. The shared
recorddbl fixture defines FINDER_DBLCLICK_TICKS=3000, but the current
finder_desktop.c click tracker reads flair_double_click_ticks (event.c's
runtime default is 20). No neighboring double-click/tolerance fix was made.
record-flair-repro was not run on this rejected scene.

The final readiness wait has not been re-recorded: the preceding two attempts
already establish the unrelated double-click blocker, before any worksheet
input. The bead is not complete: PM resolution of the shared recorder's
double-click behavior is needed before an accepted, reproducible Rule-14 clip.

## Files changed

- os/milton/kmain.c
- os/flair/system_key.h (new)
- harness/proptest/test_flair_routing.c
- harness/proptest/flair_routing_stubs.c
- Makefile
- harness/diff/dbf_diff/test_samir_dbsafe_emu.c
- harness/fixture_volumes.mk
- harness/emu/flair_launchkey.mk (new)
- spec/flair_launchkey_traces.mk (new)
- docs/worklog/launchkey-rex3.md (new)
