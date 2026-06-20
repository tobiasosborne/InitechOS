<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0044 -- DOS-3.3 parity CAPSTONE: 40oq Appendix-A INT 21h coverage certificate

**Type:** Milestone capstone (coverage oracle) + a real coverage gap closed. **Date:** 2026-06-20. **Branch:** command-com-default. Closes the orchestrated DOS-3.3 parity push (Waves 1-5, WL-0040..0044).

## Context

After Waves 1-4 (xw1/bo40/x3mh; mvg/509.7; 6zd9; p96i) the doable `40oq` capstone deps were exhausted. The capstone is the milestone closer: a coverage oracle that asserts every Appendix-A INT 21h AH reaches a real handler, and SURFACES the operator-waived/deferred go/no-go (FCB; partitions+multivol). This is achievable now and consistent with the operator's deferral intent (kzfs deferred 2026-06-15 for lack of a consumer) + the FCB precedent.

## What changed

- **`initech-40oq` -- the Appendix-A INT 21h coverage certificate.** `os/milton/test_40oq.c`: a STATIC manifest (every recognized AH from `ah_is_listed` is partitioned DISPATCHED vs operator-WAIVED FCB 0x0F-0x24; mutually exclusive + exhaustive; ah_is_listed matches the locked register) + a DYNAMIC safe-set drive (the pointer-safe query AHs reach a real handler -- no `not-yet-impl AH=` on a recording CON sink; the FCB range DOES hit not-yet-impl, proving the waiver is real). Prints the certification summary. 265 checks; mutant `CERT_MUTATE_DROP_GETVER` un-dispatches AH=30h -> the dynamic check goes RED.
- **A real coverage gap closed (found by building the cert).** AH=03h (AUX INPUT), 04h (AUX OUTPUT), 05h (PRINTER OUTPUT) were RECOGNIZED by `ah_is_listed` but UNDISPATCHED (fell to the not-yet-impl arm). Implemented `do_aux_input`/`do_aux_output`/`do_prn_output` in int21.c routing through the 509.7 device seam (`g_devio`, guarded by `g_devio_bound`). Now ZERO Core+Resident Appendix-A AHs remain in the not-yet-impl arm.

## Certification result (what the cert prints, GREEN)

```
40oq: Appendix-A INT 21h coverage CERTIFIED -- 75 recognized AHs, 55 dispatched,
FCB(0x0F-0x24) WAIVED (operator 2026-06-14), partitions+multivol DEFERRED
(kzfs/slvd, operator 2026-06-15). Functional scope COMPLETE.
  DISPATCHED Core+Resident : 55 AH(s)
  WAIVED Legacy FCB        : 20 AH(s) (0x0F..0x18+0x1B..0x24; 0x19/0x1A are
                                       dispatched Core fns inside the range)
  NOT_IMPL (unwaived gap)  : 0 AH(s)
```

The subsystem deps the capstone requires are proven by their own green gates and REFERENCED by the cert: subdirs (test-fat-subdir), FAT16 (test-fat16), MCB (test-mcb), absolute-disk (test-absdisk), INT 24h wired (test-int24-wired/mvg), break (test-4tw), wildcard (test-80k). The ONE deferred subsystem is partitions+multivol (kzfs/slvd).

## Acceptance / state

- `make test-unit` = **240 host gates** (was 238): + test-40oq(+mut). Focused emu regression (shell/exec/autoexec/samir-boot/conin/datetime) GREEN -- the additive int21 dispatch change (AUX/PRN) is safe. Kernel-end guard OK (_kernel_end=0x2f8e0 < 0x2ff00). ASCII-clean.

## The capstone verdict

**The INT 21h Appendix-A FUNCTIONAL surface is feature-complete and CERTIFIED.** The literal-total (100% incl. the Legacy class) awaits two operator-deferred items the cert formally surfaces:
1. **FCB (509.9, 0x0F-0x24)** -- operator-WAIVED 2026-06-14; its flagship consumer is the TPS Report Generator (8479.1). Dispatch it for literal-total.
2. **partitions+multivol (kzfs MBR-partition + slvd multi-volume)** -- operator-DEFERRED 2026-06-15 ("no in-tranche consumer; revisit when FAT16-HDD/multi-volume land"). slvd's SELDISK/GETDISK (0x0E/0x19) ARE dispatched (single-drive); full multi-volume + partitioned mounts are the deferred capability.

These are the operator's go/no-go. Until then, the DOS-3.3 personality (MILTON) is functionally complete for the north-star (Turbo Initech needs heap/EXEC/subdirs/handle-I/O; InitechBase needs lseek/RENAME/date-time/FINDFIRST -- all green).

## The 5-wave push (WL-0040..0044) -- what landed, all pushed

Wave 1 (WL-0040): xw1 .BAT/AUTOEXEC, bo40 AH=31h KEEP, x3mh ANSI FSM. Wave 2 (WL-0041): mvg INT 24h critical-error, 509.7 device chain. Wave 3 (WL-0042): 6zd9 device-chain INT 21h wiring. Wave 4 (WL-0043): p96i ANSI CON wiring + the FAT-cache share (after a committee ruling that broke SAMIR was caught + reversed). Wave 5 (WL-0044): the 40oq capstone. **224 -> 240 host gates; full emu (39) + Bochs green throughout.** Beads closed: xw1, bo40, mvg, 509.7, 6zd9, p96i, x3mh, 40oq (functional). ~12 follow-ups filed.
