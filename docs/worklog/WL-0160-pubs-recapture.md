# WL-0160 -- Initech Publications re-capture after the 2026-10-17 merges

## Context

Lane pubsrecap (Claude Opus, taking over a stopped Fable agent; brief
pubsrecap.md ADDENDUM + TAKEOVER). Goal: bring every manual's captured
screens back in step with the shipped OS as a deliberate, reviewed
re-capture; turn the TPS book's three hand-written DRAFT transcripts into
captures; make `make pubs`, `test-pubs` (all legs), `test-pubs-mutant`,
`test-pubs-repro(-mutant)` and `test-pubs-families` green.

The predecessor left no commits in this worktree. Its finished ADDENDUM
work (the Reference Appendix A details for MSG-DOS-0027..0060) was on
lane/pubsslips (90dcdecc) and was merged here, then corrected where it
disagreed with the source (below).

## What changed

Message appendix (ADDENDUM):
- `tools/pubs_msgtable.c`: a catalogue text beginning with `,` is the tail of
  the previous id's message (MSG-DOS-0044 "Cannot CHDIR to " + path +
  MSG-DOS-0045 ", tree past this point not processed", chkdsk.c): printed as
  ONE entry `Cannot CHDIR to path, tree past this point not processed` (the
  IBM DOS 3.10 Reference p.7-48 form); a trailing space prints `\ph{n}`
  (MSG-DOS-0043 "on cluster n"); the CR LF break no longer leaves a space.
- Reference Appendix A: details for 0027..0062 checked against the code.
  Corrected from the pubsslips text: BREAK (the DOS 3.3 disk-operation check
  is a DRAFT of the BREAK entry, not shipped), 0035 (FOR file names, not
  variable names), 0038 (ATTRIB/TREE/XCOPY, not LABEL), 0044 (/F does not
  repair it; no lost-chain recovery), 0059/0060 (what FIND and SORT check).
  New 0062 Duplicate file name (REN, lane doscmds). "Other Messages" keeps
  the three strings the catalogue lacks.
- `initech-syntax.sty`: the second line of a two-line message stays at the
  head margin. `initech-manual.cls`: the typewriter face has
  `Ligatures={TeXOff,CommonOff}` -- "--" printed as an en dash and "fi" as a
  ligature in message headings (NT-16 character for character).
- Reference CALL text: nesting too deep is now `Insufficient memory`
  (MSG-DOS-0037, dosconf lane).

Integration breaks on main that blocked the gates (fixed here):
- Every Turbo Initech book: `\MakeBackCover` defined twice (initech-forms.sty
  identity lane + initech-borland.sty). Full-size books take the family back
  cover; the PK-11 booklet keeps the borland one. (The InitechBase clash was
  fixed on main by the PM, 56ac97dd, while this lane ran.)
- TPS book: `\Response` defined twice (initech-xbase.sty, loaded by the class
  for every book, and initech-ops.sty); ops now replaces it.
- Eight books printed the company banner twice on the title verso (the
  identity lane's TitleVerso sets it, AN-27; the books also set it). The
  explicit ones are removed; the class banner fits the booklet measure
  (the Turbo QR was overfull by 48 pt).
- test-pubs-commands RED SHIPPED-NOT-DOCUMENTED since the formatsys merge:
  Reference entries for DISKCOMP, DISKCOPY, FDISK, FORMAT, SYS written from
  diskutil.c / fdisk.c / WL-0141 / WL-0147 (prose examples, like FIND:
  the capture route has no second diskette or fixed disk).
- REN entry: wildcards and `Duplicate file name` (doscmds). COPY footer
  `File(s) copied` in the prose; the COPY entry carries a DRAFT naming
  initech-bc2j (wildcards, + lists, /A /B /V, no-target are shipped).
- Journey route: since the DOS folder (WL-0144) the flagship AUTOEXEC.BAT
  skips the date/time prompts, so pubs.mk's cut of the exit trace no longer
  matched and every TPS capture failed. The cut is `,kd,ki,kr,kret`; flair
  .keys start with `CLS` (capture.sh enforces it).

InitechBase books (main moved under the lane: samirdml 14a8fc75, samircore
074725d4, samirfns 1016e8c9/a8d8cd73):
- test-pubs RED SHIPPED-NOT-DOCUMENTED for ERASE INDEX LABEL REINDEX RENAME
  RESTORE SAVE SORT TOTAL TYPE and SET ALTERNATE: Volume One entries (Syntax
  and Usage from the code, no examples yet) and Quick Reference lines.
  Message entries 7, 23, 48, 54, 55 were "no command raises"; they are now
  raised and say by what. SET CONSOLE documented as shipped (initech-vz6z).
- 258 example transcripts re-minted (`make pubs-examples`), 127 changed:
  real-dBASE output parity (`?` numbers in ten columns, LIST/DISPLAY numeric
  headings right-aligned and logicals as .T./.F., sums keep decimals). Six
  examples and the sample program AMTCHECK.PRG used TYPE(<exp>); the shipped
  TYPE() is the real III+ TYPE(<expC>) (Data type mismatch otherwise), so
  the TYPE() entry, the five examples and AMTCHECK.PRG use the string form.

TPS book (step 3):
- R2/R5 wording shipped (77835faa): `\TpsDraftMsg` -> `\TpsMsg`,
  `\TpsDraftMessageEntry` -> `\TpsMessageEntry`, the "prompt displayed again"
  DRAFT removed, `\DraftShot` -> `\OpsShot` for r2align / r5cover / r5refuse,
  .draft files deleted, three SCREENS rows added.
- r5cover cannot end on a DOS prompt (Y scrolls the prompt away with the
  report), so capture.sh gained one settled state: a marker `=TEXT[#N]`
  means the program waits at its own prompt and the last screen line must
  END with TEXT. The capture stops at the cover-sheet prompt.

## Screen drift, each explained

| Screen(s) | Change | Cause (merge) |
|---|---|---|
| Ref chkdsk, chkfile; UG chkdsk | 118320 -> 118448 bytes memory | CHKDSK.COM 8369 -> 8241 bytes (arena = ceiling - image end), dosconf 8b639660; built both to check |
| Ref/UG DIR, CHKDSK, edlin, md, ren, del, dirargs, cdsub, backup, paths, copy | practice diskette free -1024, user bytes +1024 | EDLIN.COM 7185 -> 8209 bytes (15 -> 17 sectors), conkeys e91a8c3d + dosconf |
| Ref copy, del; UG copy, paths; TPS tps002 | `file(s) copied` -> `File(s) copied` | MSG-DOS-0061, doscmds 0112ab83 |
| InitechBase gs-dir | sample diskette free 1314816 -> 1303552 | SAMIR.COM grew (samircore 074725d4, samirdml 14a8fc75); sample files unchanged |
| TPS tpsdir, tpsbak | flagship volume free -80896 | \DOS folder, NOTEPAD/SCRAPBK, CPANELS, FDISK, FORMAT/SYS/DISKCOPY/DISKCOMP added to the data volume |
| TPS tpsstart, tpsritual | R2 prompt and R5 prompt text | tpsr2 77835faa (expected, WL-0139) |
| InitechBase gs-dir (second pass) | +7168 B free | SAMIR.COM 105,087 -> 97,915 B (size round 1, 1016e8c9) |
| TPS r2align, r5cover, r5refuse | new captures | replace the DRAFT transcripts |

No unexplained change.

## DRAFT markers

TPS book 11 -> 3 (address check, MSG-DOS-0006/0008 kept). Reference 11 -> 12
(+1 COPY, initech-bc2j).

## Follow-ups

initech-bc2j (COPY entry rewrite), initech-lstk (fi ligature in the other
mono faces), initech-pian (diskutil/fdisk messages in Appendix A),
initech-8q0k (InitechBase: examples for the new entries, APPEND FROM / CLOSE
<type> / COPY FILE / CLEAR MEMORY / macro &, the u7 raised-message audit).

## Pointers

tools/pubs_msgtable.c; pubs/screens/capture.sh; pubs/pubs.mk (PUBS_FLAIR_EXIT);
pubs/manuals/tps-report-generator/screens/SCREENS;
pubs/manuals/initechdos-reference/appa-messages.tex, ch2-entries-*.tex.
