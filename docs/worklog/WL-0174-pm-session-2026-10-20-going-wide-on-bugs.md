# WL-0174 -- PM session 2026-10-20, 02:00-07:00: going wide on bugs, merging as we go

## Context

Operator directive at 02:10: increase velocity -- fan out Codex and Opus subagents (occasional
Fable) on tightly scoped one-job lanes, fix bugs, grind the beads toward the final app, MERGE
(no hundreds of unmerged branches), coordinate with the peer Claude session (sublimite-19, a
different repo sharing the box and the account quota). Quota rules agreed with the peer:
/usr/bin/quota before every dispatch, no Codex launch at or above +9 % ahead of pace (the
operator allowed about +10), Fable subagents only while Fable weekly <= Claude weekly + 2.
Box load from both sessions sat at 20-40 until about 04:30 and shaped the emulator work.

## What changed on main (33 lane merges, in order; PM commits between them)

| Lane | Model | Beads | What |
|---|---|---|---|
| dsou | Codex | dsou | DOS keyboard: two Shift bits + CapsLock latch |
| lexer | Codex | veav, cfc5 | seed integer literals out of 32-bit range are a located error |
| shellflake | Codex | 7r2 | harness waits keyed to guest progress (QEMU CPU-time idle budget) |
| pubsaux | Codex | 46u4 | stale TeX auxiliaries cleaned on class/style change |
| cursor2 | Fable | vzjy | the arrow on the interactive desktop at boot (320,240) |
| f3template | Codex | iw0h | F3 command template recall, extended keys as 00,scan pairs |
| fatspc | Codex | fol7 | FAT clusters of 2..64 sectors through one-sector scratch |
| nestexec | Codex | n8q8 (1) | DOS-3.3-true nested EXEC from a resident program |
| adrcite, adr13 | Codex | 0ni0, t5xs | ADR citation gate over every Record; ADR-0013 AC-3 |
| fixlint | Codex | o0cb | fixture-volume lint covers Bochs legs under SKIP_BOCHS |
| pubsslips | Opus | ipu5.16 | VO-30 house register of tolerated inconsistencies |
| kstack | Opus | xu97 | kernel stack in a disjoint high window + 45-pair map guard |
| launchkey | Codex | rex3 | SAMIR launcher is Ctrl+Alt+backslash; backslash reaches the sheet |
| i123entry | Codex+Opus | twxf | 240-character Initech 123 entry line |
| samirrun | Opus | n8q8 (2) | SAMIR RUN / ! through the PAL exec seam and COMMAND /C |
| ctenant | Opus | 6qsk | CTENANT paints readouts from retained state |
| egg | Codex | 2ter (a) | the Jump to Conclusions Mat (Option-click the Apple icon 3x) |
| samirbochs | Codex | hdlb | RFB key replay in the Bochs driver + the SAMIR Bochs leg |
| shutdownbudget + orphan | Codex+Opus | 64d6, 89sb | power gates on an 800-tick image; the orphan cluster was a harness race |
| recorder | Codex | 9gxs | record images' double-click interval reaches the Event Manager again |
| sidekick, skresearch | Opus | 6fdt (1)(2) | TSR mechanics (AH=31h/34h, idle 0x38, page save 0x39); SideKick grounding |
| finderalerts | Opus | tdnl.61-.88 (11) | Finder limits, alerts, input + kernel size round 6 |
| samirfix | Opus | ipu5.8.1 (1) | InitechBase error numbers, ON ERROR, MESSAGE() |
| menukeys | Codex | ptzy | --keys-ack: per-key guest acknowledgement in the QEMU harness |
| pubsrecap | Opus | 090x | InitechBase books in step; DISKCOMP; screens re-captured |
| size7 | Opus | 5qsm | kernel size round 7: 183,660 -> 176,756 B |
| geneva12 | Opus | uelk | Geneva 12 strike; InitechText on 16-px lines |
| iwordp3 | Opus | qvgd | InitechWord List Files (F5) from a real WP 5.1 mint |
| i123keys, i123hit, i123resize | Opus | 69mz, umfi, 89rc | EDIT/GOTO/PgDn/Tab; cell hit testing; real resized geometry |
| basebooks | Opus | 8q0k | InitechBase books: 18 replayed examples, RUN/!, &, COPY FILE, CLEAR MEMORY |

Beads closed this session: hngh veav cfc5 dsou oby0 7r2 iw0h 46u4 vzjy fol7 0ni0 t5xs o0cb
ipu5.16 xu97 rex3 twxf n8q8 6qsk 9gxs hdlb 64d6 89sb ptzy 5qsm zxb7 69mz umfi 89rc (29).

## Decisions (PM-ratified pending operator)

- SAMIR RUN golden: VERSION.TXT is DOSBox-X's VER banner (impossible by construction); graded
  as created-by-the-redirected-child + byte-identical to InitechDOS's own VER > file; GOLD.OUT
  stays byte-exact vs real III+ (n8q8). Shell kernels route EXEC COMMAND.COM to the in-kernel
  COMMAND /C builtin; flagship guard -> initech-13ig.
- TSR vectors: idle 0x38, screen service 0x39 (DEC-04a.2 keeps IRQ0 at 0x28); spec/tsr_contract.h;
  AH=34h register row. Pilot allowlist growth rows for INT21:32 and INT21:34 (undocumented in
  DOS 3.3, await the Technical Reference) -- named, not loosened.
- The arrow at boot on INTERACTIVE images only (mint-results-009 Sec 1); gate images keep the
  first-activity DEC (vzjy).
- Known-red gates stay NAMED: test-bochs-pending (the SAMIR Bochs leg's RFB shadow catches the
  cursor blink phase, initech-hgmx).
- Routing legs C4/C6 and solid leg E re-keyed to the merged Finder contracts (tdnl.65: the
  activating click is consumed; tdnl.79: demo items are disabled).
- 123.EXE build takes -fomit-frame-pointer and the Round-5 pass switches (64 KiB cap kept);
  Initech 123 E9 is two Escs (the real R2.2 capture).
- SideKick period truth vs the bead text: no paper tape in 1.5x, F1-F7 + Esc, no SK options.
- PM-directed re-keys and the back-cover question are beads for the operator (f8b1, 9222,
  bkmv unchanged).

## Frictions

- Bounded-pump gates under load: the default 250-tick pump ended before injected traces finished
  and the raw ring dropped keys (FLAIR-EVT-DROP). Honest mechanisms only: gate-local images with
  FLAIR_TEN_TICK_BUDGET at ~2x the measured need; --keys-ack in the harness. Re-run alone
  before believing a red; repro proofs of clips need load1 < 2.
- Size budgeting across parallel kernel lanes: three lanes green alone put the flagship 3,436 B
  over the floor together; size round 7 (-freg-struct-return alone -4,992 B) brought it to
  176,756 B. Main was held unpushed for two hours with a backup on origin pm/main-pending.
- A scripted conflict resolver that failed its assertion was followed by a commit of a Makefile
  WITH markers (menukeys); the resolver now asserts the marker count and the commit is gated.
- The Codex sandbox cannot open the Bochs RFB socket; live Bochs legs are PM-run, serially.
- -j runs of a gate with its own mutant collide on shared scratch names (cursor-boot, bochs-legs).
- bd's embedded Dolt is single-writer: every bead write wrapped in a retry loop.
- The orphan FAT cluster after a declined Shut Down was a harness race (quit on the Ctrl-N
  keyDown before fat12_mkdir finished), proven by a mutant that reproduces the evidence image.

## Acceptance

Every merged lane re-verified by the PM on main (its families + mutants + the size floor);
Bochs legs run for kstack (boot, desktop, appswitch) and finderalerts (4 legs); test-kernel-
headroom green over 274 kernels; test-pubs green serially except the entries the basebooks
lane added (its own run green). A full `make clean && make test` certificate has NOT run on
this HEAD: first job next session (in ../initech-os-cert).

## Pointers

- PM ledger and briefs: the session scratchpad (volatile); bead notes carry the pointers.
- Open follow-ups filed tonight: 13ig (flagship EXEC COMMAND.COM), h4bk (non-PIC programs after
  a resident), vx9d (SK.COM UI), hgmx (SAMIR Bochs RFB cursor phase), ioke (SAMIR unit vector
  reds), 2trr (III+ control flow), 7z2z (kbd.c array-bounds mutant), ytsm (trash-purge-views
  budget), b9oa, dbag, f8b1 (back cover), the clips owed on tdnl.92.
- Clips: all ten owed clips record with 0 dead events; six repro-proven; locked_trash,
  trashfull_regression, finder_refused, toolbox_hardening race above load 2; the Finder alerts
  bundle's 13 scripts and the itext/listfiles clips still owe a quiet-box record.
