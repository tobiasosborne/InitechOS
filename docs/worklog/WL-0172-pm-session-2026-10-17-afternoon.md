# WL-0172 -- PM session log, 2026-10-17 afternoon and evening

## Context

Operator directive at the start of the day: use the Claude and Codex weekly
quota, keep Fable usage equal to Claude usage, keep 4 cores and ~12 GB free
for the operator, go wide toward the north star, continue the manuals
programme. Mid-afternoon the directive changed: slow down, divide the quota
evenly to the reset, equalise Fable with sequential short Fable lanes, then
back to Opus by default, one lane at a time. This shard records what the PM
merged, decided and left open from the context compaction (about 13:50) to
session close (about 23:00). Lane worklogs carry the detail; this is the
merge ledger.

## What changed on main (in merge order)

| Merge | Lane | Worklog | What |
|---|---|---|---|
| 4755d030 | wordman | WL-0135 | InitechWord Reference + Workbook + template at P3 |
| 99f22708 | samirfns (run 4) | samirfns-fourth-launch.md | SET INDEX, LIST OFF, RECSIZE(), TYPE() mutant; four real-III+ mint rows |
| 60794f14 | bootscreen | WL-0151 | FLAIR boot screen: Mac OS 8.1 Welcome panel + the Initech logo strike (mint-009); SAMIR hotkey drain fix; boot_screen clip block |
| 4d907b1f | dosvolumes (run 2) | WL-0156 | DEC-18 drive letters as real ATA volumes, FAT16 on the second, the INT 21h coverage register |
| eecdc74f | routing | WL-0165 | O-1-style host routing suite over disk_tenant_event + four mutants |
| 08389c14 | adr0002 | WL-0161 | ADR-0002 reconstructed from its citations, ADR-0001 Rev 2.1, test-adr-citations gate |
| 4edf2ee8 | fixups (Fable) | -- | panic-screen build dir, SAMIR fns goldens promoted into the tree, DOS-message vocabulary fix |
| 0ad6beec | pubsfix (Fable) | -- | four post-merge TeX reds fixed; 35 DOS message details authored; one banner per verso |
| 0d3f26fb (ff) | dpb (Fable takeover) | WL-0167 | INT 21h AH=1Fh/32h Get DPB vs minfo/mdir; AH=32h register row RATIFIED by the operator; coverage certificate back in the default vector |
| 64fa4062 | samirsize (Fable) | WL-0171 | SAMIR.COM -4,077 B by codegen policy, no behaviour change |
| 12112754 | ksize5 (Fable takeover) | WL-0168 | flagship kernel -11,040 B (ABI, pass policy, packed strikes, part map); 16 KiB floor green |
| 1d475ce3 | bochs3 (Fable takeover) | WL-0170 | Bochs legs re-keyed to the VESA 0101h path, graded pixel-exact vs QEMU; forced VGA13 legs on ISA Cirrus / plain VGA |

PM commits between merges: the panic-screen Bochs diskette contract
(104bff99), test-samir-fns split into reproduced vs pending cases
(e71ed1c6), test-samir-dml RUN moved to a pending target (3c32c4df), the
Bochs leg macro's explicit fifth argument at 42 call sites (858e6df6),
WL-0167 ratification note (104f9e7f).

## Decisions

- The operator RATIFIED the AH=32h GETDPB row in spec/int21h_register.json
  and ADR-0003 Appendix A (2026-10-17, this session).
- Known-red gates leave the default vectors as NAMED targets with a bead,
  never as a loosened oracle: test-samir-fns-pending (13 scaffolding
  cases, initech-9dyu), test-samir-dml-pending (RUN, needs the nested EXEC
  seam, new bead), test-kernel-coverage / -certificate were preflights
  for three hours until the DPB lane landed and restored them.
- Merges happen only between family runs on main. A Makefile with conflict
  markers for two minutes made every gate launched in that window rc=2
  (the batch-4 cascade). Recorded with `bd remember`.
- Quota: one lane at a time while the 5-hour session is high; Fable and
  Claude weekly percentages kept equal (37/38 at close).

## Frictions

- The record-flair clips with injected interactions all miss their
  interaction marker on main since the boot-screen merge (grow_box,
  finder_list_view, dos_folder, locked_trash, trashfull_regression,
  iword_search, iword_replace, toolbox_hardening); boot_screen and
  note_pad record and repro-prove. The bootscreen agent was re-tasked on
  lane/bootscreen2 and paused before a fix; evidence in the scratchpad
  clips-1.log and the cert clone's build/clips/rec_grow_box.*.
- The mouse cursor is not visible in an interactive QEMU window (operator,
  three reports). Bead filed with the evidence: the sprite is shown only
  after the first PS/2 packet; the splash-to-desktop repaint may erase it.
- The samirfns run found real dBASE III PLUS prints separators for a blank
  DTOC() and rolls invalid month-ends forward while the existing date
  references expect spaces/blank: CLAUDE.md stop condition, bead
  initech-bkmv, operator decision pending.
- Codex lane i123p4 ran to session close and was stopped mid-run; its
  state is a wip commit on lane/i123p4 (UNVERIFIED).

## Acceptance (main at close)

- Last full family on main (batch 5, HEAD 4d907b1f): 46 green / 9 red; every
  red since fixed or moved to a named pending target.
- Gates re-run on main after the later merges: test-makefile-vars,
  test-adr-citations, test-flair-routing, test-kernel-coverage (76 AHs),
  test-samir-fns (7 cases), test-samir-size, test-samir-dml (15 verbs),
  test-kernel-size-r4, test-bochs-legs (72 legs), test-flair-desktop-bochs
  (307200 px exact), test-boot-bochs: all green.
- A full `make clean && make test` certificate has NOT run on the final
  HEAD; it is the first job of the next session (in ../initech-os-cert).

## Pointers

- Lane branches pushed to origin as lane/* (81 refs) so nothing lives only
  on this machine. Paused lanes with UNVERIFIED wip commits: finderrobust,
  samirfix, mints, pascaltbx, dupinfo, i123p4; finished but unmerged:
  pubsslips (3 conflicts to resolve against main), finderalerts (see its
  WL-0162 for the size-round-6 state), l123man / flairman / flaircmd /
  pubsrecap / iwordp4 (Opus lanes stopped mid-work; their worktrees under
  .claude/worktrees/lane-*).
- Briefs for every lane: the session scratchpad briefs/ directory (volatile;
  the bead notes carry the pointers).
- Operator decisions open: initech-bkmv (SAMIR dates), initech-9222
  (desktop teal vs Initech Cyan), the ten physical-edition decisions in
  docs/packaging/MANUALS-AND-BOXES-RESEARCH.md Sec 9, the astra clip fetch.
- New beads this session: initech-6fdt (Initech SideKick), initech-2ter
  (Jump to Conclusions egg), the ADR citation widening, the SAMIR RUN /
  nested EXEC seam, the mouse cursor bug.
