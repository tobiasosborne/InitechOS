# WL-0173 -- I123 menu input backpressure (initech-ptzy)

Lane: `/home/tobias/Projects/initech-os-menukeys`, `lane/menukeys`.
Changes remain in the working tree for PM review. No beads writes or git
mutations. `bd show initech-ptzy` returned `no issue found matching
"initech-ptzy"`; the supplied lane assignment is the task authority.

## Ground truth and cause

Read the main tree's CLAUDE.md top to bottom, PRD Sec 6.3/8,
spec/event_model.h, spec/flair_i123_traces.mk,
spec/flair_i123_menu_traces.mk, spec/i123/menu_tree.golden and the existing
menu gate. The RI tree and pixel/row oracles are unchanged.

The harness's 30 ms (`--keys`) / 40 ms (`--mouse`) QMP drains acknowledge
neither IRQ delivery nor task-context consumption. All 50 menu strokes can
arrive while the cooperative guest is still loading/redrawing. Its locked
raw ring holds 64 records (one per scancode byte), not 64 strokes. Seven extended Rights
need four records each, and the two Shift chords add modifier records:
the complete keyboard stream needs 118 records.

Both before captures have **identical QEMU keyboard transitions** (addresses
normalized): 52 makes, 52 breaks (50 strokes plus two Shift presses), and
118 keyboard data-port reads. In particular QEMU emitted all five consecutive
Right make/break pairs. The guest reports `FLAIR-EVT-DROP` in both runs, and
only 29 normal / 42 mutant keyDowns survive. This localizes the observed loss
to raw-ring overflow, after QMP/PS2 delivery and before tenant dispatch.
The recorded QMP transition diff is empty. This is not evidence of QMP
held-key collapse or Event Manager typematic folding; ordinary autoKey
generation is explicitly deferred in harness/proptest/test_event.c.

The reported exact `00002000` sequence did not recur in these captures.
Its decoding is consistent with losing a byte after an E0 prefix: event.c
cooks `E0 20` as virtual key 0x20 with ASCII zero, giving `00002000` instead
of `00002064`. That explanation of the earlier particular byte value is an
inference from the decoder, not a claim to have recaptured those bytes.

The mutant-only premise did not hold on this loaded box: the normal gate
failed twice, including its required repeat alone. In the traced pair, normal
lost more keys than mutant. Both boots use the **same**
flair_tenants_mut_i123_menu_gate.img, the same flags, the same 1,000-tick
pump budget and the same loaded app base 0x001a6750. Both data images are
1,474,560 bytes. Their 123.EXE differs: normal 65,376 bytes (load module
62,192), mutant 65,008 (load module 61,840), built with I123_MUT_MOVE_STALE.
This changes optimized code/layout and scheduling relative to the timed burst.
The missing referrer-update branch runs only at /Move completion, after the
earlier input loss. No semantic pre-Move event difference explains it.

Before evidence: build/menukeys-{normal,mutant}-before serial captures are
named `menukeys_normal_before.serial` / `menukeys_mutant_before.serial`;
QEMU traces are build/menukeys-ps2-before.log and
build/menukeys-ps2-before.log.872. Normal baseline logs are
build/menukeys-baseline.log and build/menukeys-baseline-alone.log.

## Change

Opt-in `--keys-ack 'FLAIR-EVT what=4 '` snapshots the marker count **before**
sending each stroke, and waits for a new guest release echo before the next
stroke. It covers --keys, --mouse k chords, and explicit K releases. An old
echo or QMP reply cannot release the barrier. Missing acknowledgements stop
input and fail the run. The wait reuses the existing guest CPU/progress idle
budget and shared 120-second ceiling. Bare modifier releases are unsuitable
for this option because Event Manager intentionally swallows them.

Only the menu family opts in. Its original trace, 1,000-tick life, timeouts,
goldens and pixel tolerances stay fixed. The gate additionally rejects ring
drops, requires all 50 keyDowns, and diffs normal and mutant keyDown streams
before accepting the stale-H1 mutant. No os/ source changed.

Paced strokes use explicit make/break, releasing chord members in reverse
order, and replace the inter-key fixed drain with the acknowledgement wait.
Unpaced callers retain send-key and the original drain. Waiting for echoes
while still using send-key's implicit 100 ms hold passed two full runs but
then exhausted the unchanged pump life after 37 keys; its required repeat
alone exhausted it after 36. Both ended with FLAIR-LIVE-OK, no ring drop,
and a missing-release-echo failure. This was corrected by removing that
implicit hold from the paced path, not by adding time or changing the oracle.
Logs: build/menukeys-menu-pass3.log and
build/menukeys-held-timer-repeat.{log,serial,report}. The host regression also
rejects send-key's implicit timer on the paced path.

The host regression first went RED with a scripted 175 ms consumer:

```
RED qemu-key-ack: --keys must wait for each new guest release (sent=3 writes=3 early=2 at=90)
RED qemu-key-ack: --mouse k must wait for each new guest release (sent=3 writes=3 early=2 at=120)
RED qemu-key-ack: explicit K release must wait too (sent=4 writes=4 early=2 at=160)
RED qemu-key-ack: missing echo stops before key 2 (sent=3 writes=3 early=2 at=120)
```

With pacing it passes stale echo, slow echo, fast echo during QMP drain,
explicit K release, missing echo and opt-out cases. The one-branch
HARNESS_MUTATE_NO_KEY_ACK mutant restores the same premature-input RED.

## Token comparison

All 50 locked keyDown tokens and expected messages are listed below. Before:
`N`/`M` are the normal/mutant **captured event ordinals**, `-` means absent
in a longest-common-subsequence alignment. Identical repeated keys cannot
identify *which* of the five physical Rights survived; exactly one survived
in each run. After pacing, every row matches its same ordinal and expected
message in both captures, with no raw-ring drop.
The after column is the first paced capture (which still used send-key's
hold timer), not a certificate of three passes for the final implementation.

| Token | Key | Message | Before N/M | After N/M |
| --- | --- | --- | --- | --- |
| 1 | 1 | 00000231 | 1/1 | 1/1 |
| 2 | 0 | 00000B30 | 2/2 | 2/2 |
| 3 | Right | 00004D00 | 3/3 | 3/3 |
| 4 | - | 00000C2D | 4/4 | 4/4 |
| 5 | a | 00001E61 | 5/5 | 5/5 |
| 6 | 1 | 00000231 | 6/6 | 6/6 |
| 7 | Shift-8 | 0000092A | 7/7 | 7/7 |
| 8 | 2 | 00000332 | 8/8 | 8/8 |
| 9 | Right | 00004D00 | 9/9 | 9/9 |
| 10 | - | 00000C2D | 10/10 | 10/10 |
| 11 | b | 00003062 | 11/11 | 11/11 |
| 12 | 1 | 00000231 | 12/12 | 12/12 |
| 13 | Shift-8 | 0000092A | 13/13 | 13/13 |
| 14 | 2 | 00000332 | 14/14 | 14/14 |
| 15 | Return | 00001C0A | 15/15 | 15/15 |
| 16 | / | 0000352F | 16/16 | 16/16 |
| 17 | c | 00002E63 | 17/17 | 17/17 |
| 18 | b | 00003062 | 18/18 | 18/18 |
| 19 | 1 | 00000231 | 19/19 | 19/19 |
| 20 | . | 0000342E | 20/20 | 20/20 |
| 21 | . | 0000342E | 21/21 | 21/21 |
| 22 | c | 00002E63 | 22/22 | 22/22 |
| 23 | 1 | 00000231 | 23/23 | 23/23 |
| 24 | Return | 00001C0A | 24/24 | 24/24 |
| 25 | d | 00002064 | 25/25 | 25/25 |
| 26 | 1 | 00000231 | 26/26 | 26/26 |
| 27 | Return | 00001C0A | 27/27 | 27/27 |
| 28 | Right | 00004D00 | -/- | 28/28 |
| 29 | Right | 00004D00 | -/- | 29/29 |
| 30 | Right | 00004D00 | -/- | 30/30 |
| 31 | Right | 00004D00 | -/- | 31/31 |
| 32 | Right | 00004D00 | 28/28 | 32/32 |
| 33 | - | 00000C2D | -/29 | 33/33 |
| 34 | d | 00002064 | -/30 | 34/34 |
| 35 | 1 | 00000231 | -/- | 35/35 |
| 36 | Return | 00001C0A | -/31 | 36/36 |
| 37 | / | 0000352F | -/32 | 37/37 |
| 38 | m | 0000326D | -/33 | 38/38 |
| 39 | d | 00002064 | -/- | 39/39 |
| 40 | 1 | 00000231 | -/34 | 40/40 |
| 41 | . | 0000342E | -/- | 41/41 |
| 42 | . | 0000342E | -/35 | 42/42 |
| 43 | e | 00001265 | -/36 | 43/43 |
| 44 | 1 | 00000231 | -/37 | 44/44 |
| 45 | Return | 00001C0A | -/38 | 45/45 |
| 46 | f | 00002166 | -/39 | 46/46 |
| 47 | 1 | 00000231 | -/40 | 47/47 |
| 48 | Return | 00001C0A | 29/41 | 48/48 |
| 49 | / | 0000352F | -/42 | 49/49 |
| 50 | r | 00001372 | -/- | 50/50 |

The first paced pair's final whole-row draws differ only at H1:

```
normal: s="      10      -20       40                        -80      160       80 "
mutant: s="      10      -20       40                        -80      160        0 "
```

See build/menukeys-token-diff-{before,after}.txt and
build/menukeys-{normal,mutant}-after-pass1.serial for the complete captures.

## Acceptance and stop

**Not complete.** The final explicit-transition implementation has not passed
the required three full menu runs. Its first run reached the unchanged pump
cutoff after 41 correct OPS keys, without ring loss. The required repeat alone
completed all 50 OPS keys and passed the pixel gate, but fragment 5 reached
the cutoff after 46 of its 48 keyDowns. The existing level checker then failed:

```
[harness] --keys-ack: new release echo 'FLAIR-EVT what=4 ' missing; stopping input
missing menu entry: /Print/Printer/Options/Margins
make: *** [Makefile:31910: test-flair-i123-menu] Error 1
```

Evidence: build/menukeys-final-menu1.log,
build/menukeys-explicit-budget.{serial,report},
build/menukeys-final-menu-repeat.log,
build/menukeys-explicit-complete-ops.serial,
build/menukeys-explicit-fragment5.{serial,report}. The OPS pixel line in the
repeat is `ppm_text_check: i123-menu bands=1491/1479/433 MENU=330 navy/182
white: green`.

The original test-flair-i123 also failed twice, including the repeat alone.
Its unchanged 250-tick pump ended first after `He`, then with `Hello`, `123`
and `1.5` present but `-7` absent. Both runs reached FLAIR-LIVE-OK. Its
ARROWS_DEAD mutant recipe did not run because test-flair-i123 is a prerequisite.
No neighbouring test or source was changed to make this pass.

```
!!! test-flair-i123 [1/3] FAIL: TYPE -- 123 did not come up, or row 1 / line 1 / the mode / the pointer are not what the typed MINT01 row must give
make: *** [Makefile:23146: test-flair-i123] Error 1
```

Logs: build/menukeys-i123.log and build/menukeys-i123-alone.log. Host load was
roughly 21-34 on 12 CPUs during these checks. A quiet acceptance slot or a PM
decision about guest timing is needed; no timeout, pump budget, clock mode,
golden, diff or pixel tolerance was loosened. Bochs and the full make test
vector were not run, as instructed. No clip was minted for this harness fix.

Direct make targets run (all with -j3; every QEMU invocation additionally set
INITECH_QMP_STDIO=1):

* test-flair-i123-menu-mutant (includes test-flair-i123-menu): baseline RED
  twice; two intermediate held-timer passes; then the held-timer budget RED
  plus repeat; final explicit-transition RED plus repeat, as above. The two
  intermediate logs, build/menukeys-menu-pass{1,2}.log, contained:

```
>>> test-flair-i123-menu: green (tree, RI descriptions, Copy/Move row and MENU pixel bands)
>>> test-flair-i123-menu-mutant: green (Move left H1 stale: 0 instead of 80, correctly RED)
```

* test-flair-i123 and test-flair-i123-mutant: TYPE RED and repeat RED above;
  mutant blocked by the failed prerequisite.
* menukeys-before, via the untracked build/menukeys_diag.mk: diagnostic only,
  captured the normal/mutant pair; it has no pass oracle or green claim.
* test-harness-key-ack: initial RED shown above, final green below. An extra
  test refinement briefly failed when its synthetic QMP producer used send()
  in the restricted runner; it now uses the local socketpair's write().
* test-harness-key-ack-mutant, test-harness-progress,
  test-harness-progress-mutant, test-event, test-event-mutant,
  test-makefile-vars, test-kernel-size-r4: final verbatim gate lines below,
  from build/menukeys-host-final.log and build/menukeys-quality.log.

```
>>> test-harness-key-ack: green (both streams wait for new release echoes; explicit K, fast echo, missing echo and opt-out)
>>> test-harness-key-ack-mutant: green (NO_KEY_ACK correctly RED: next key outruns guest consumption)
>>> test-harness-progress: green (serial progress renews the deadline; missing/partial/count/ceiling/exit stay bounded)
>>> test-harness-progress-mutant: green (disabled progress-reset branch went RED)
>>> test-event: green
>>> test-event-mutant: green (DROP_SYNTH correctly RED)
>>> test-event-mutant: green (STALE_WHERE correctly RED)
>>> test-event-mutant: green (SET1_BREAK_BLIND correctly RED -- released modifiers latch)
>>> test-makefile-vars: green
>>> test-kernel-size-r4: green (flagship keeps >= 16 KiB headroom in the 384-sector window)
```

Final host mutation evidence:

```
RED qemu-key-ack: --mouse k must wait for each new guest release (sent=3 writes=3 early=2 at=0)
RED qemu-progress: echoes must renew the completion deadline (seen=0 at 100 ms)
```

Working tree files: Makefile, harness/emu/qemu.c, harness/emu/qemu.h,
harness/emu/qemu_main.c, harness/emu/test_qemu_key_ack.c, and this worklog.
The tree is intentionally uncommitted for PM review. No git commit, stash,
checkout, branch change, push or beads write was performed.
