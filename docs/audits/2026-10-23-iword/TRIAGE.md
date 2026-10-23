# InitechWord source triage

## Wrong claims and reference corrections

- L016 is WRONG as a system-defect claim. The shipped instruction in
  pubs/manuals/initechword-reference/ref-l.tex:87 is wrong: Esc leaves Look,
  but does not leave List Files. The real-WP LISTF ledger
  ../wordperfect51-decomp/mint/work/LISTF/mint-results-005.md:126 explicitly
  records the unchanged screen after Esc (LF03_16). Current
  os/iword/app/face_a.c:1526 matches that behavior. Verdict DOC; repair the
  instruction, preserve the real behavior.
- L023's suggestion that the arrows are enabled is not established. The
  supplied s01-scroll-arrow.png shows gray arrows; kmain.c:2310-2315 emits
  FLAIR-SCROLL-IGNORED for no active controller/part. The real gap is missing
  Word scrollbar/viewport integration (F), not a broken registered controller.
- L012's expectation is correct, but its cited wp-search.txt extraction does
  not contain the Ctrl-M instruction. I checked the original local
  WPDOS51Reference.pdf, printed p. 573/PDF p. 591: Ctrl-m includes SRt.
  The source event mismatch confirms D independently of that bad excerpt.

No other observation is rejected. L013/L014's exact historical correction
remains unminted, as the auditor already states; the existing geometry bead
must acquire that oracle before choosing a rule. L007 demonstrates missing
table display structure, not proven deletion of serialized table bytes.

## Scope and verdict discipline

Audit: docs/audits/2026-10-23-iword/{REPORT.md,NITS.tsv,evidence,shots}.
All 91 findings L001-L091 are accounted for exactly once in the table below.
Audit image: e67b8311092873fe16e6c6d62ad82f894bfb8d7d.
Source inspection: 2026-10-23; repository HEAD at start:
3e44338f53906c23e54fc747a3d689a5d7e0ee2a.
Source citations are pinned to that revision. During this read-only triage,
HEAD advanced to c995e23f7d366e5999abbc155cad41dcce6835bf via two I123 audit
commits; the inspected Word, Toolbox, kernel-event and manual source files
have no committed diff between those revisions.
Repository: /home/tobias/Projects/initech-os. Source locations are relative
to that repository; a bare face_a.c/piece.c/edit.c/codes.c in the table means
os/iword/app/face_a.c or os/iword/core/{piece,edit,codes}.c respectively.
tbxgate.c/event.c are in os/flair; kmain.c is in os/milton. References starting
../wordperfect51-decomp name the sibling real-software corpus. The original
WP manual is /home/tobias/Projects/initech-manuals-ref/wordperfect51/WPDOS51Reference.pdf.

Read CLAUDE.md, the requested 2026-10-05 triage exemplar, this audit, AGENTS.md,
PRD 1.4/6.1/6.3 and the Word plan/locked specs plus relevant source/reference
material. Source and retained evidence inspection only: no repository edits,
builds, bd mutations, emulator sessions, commits or oracle runs. The file-size
and unsupported-byte checks are read-only evidence checks, not green fix claims.
Only TRIAGE.md and BEADS.tsv are delivered in this directory.

D means broken implemented behavior, including accepted settings with no
consumer. F means no implementation at the reported entry point, even where
an API/core helper exists elsewhere. Thus an unimplemented Ctrl-End, Go To,
Block+Enter or mouse route is F, not a broken deletion/navigation/hit-test
routine. A missing setting or command is not BY-DESIGN just because the manual
honestly documents the omission. CLAUDE's deliberate canon and fail-loud policy
are preserved; no finding here attacks the hourglass, 116% pie, trailing minus,
cooperative scheduling, safe unknown-code refusal or reasoned invariant panic.
No new panic-screen defect was observed. There are zero BY-DESIGN rows.

DOC means wrong shipped documentation. DUP/LANE take precedence in the table
over underlying D/F; their notes retain the behavior and source explanation.
BEADS.tsv contains only new D/F/DOC batches, with a header and exactly five
tab-separated fields. It is a filing proposal, not executed tracker commands.

## Existing owners and exclusions

OPEN-BEADS.txt and LANES-IN-FLIGHT.txt are the authoritative supplied snapshots;
no live tracker status was assumed. L001/L002 are in lane initech-mcep
(atomicsave), so neither gets a new item. The same write_file defect destroys
the old file or leaves a new empty file; splitting off cleanup would duplicate
the dispatched mechanism. Read-only evidence confirms ORIGINAL.WP5 was 373
bytes before and zero after both S02/S04 failures. New backup/Block Save/file
operation work must use that lane's safe file primitive, not refile it.

Narrow existing mappings:

- initech-naoz: L013/L014, the explicitly named long-word Tab/Indent edge.
- initech-9m65: L010/L040 (extended-character input/rendering), L036-L038
  (Append/Tabular Column/Rectangle), L044/L045 (Base Font/Print Color).
  Its title mentions the other Move entry, but these are the same absent
  engines/selectors, not a new implementation per entry point.
- initech-8q6j: L007/L073 (tables/columns), L011/L082 (footnotes), L068
  (Print, including List Files Print observed under that finding). 9m65's
  footnote research is a dependency, not another proposed owner.
- initech-xp4k: L070/L078, thesaurus/spell-check data and delivery.

No epic is used to swallow all new Word work. initech-tu3d's prefix/D4
research does not own the missing 0x8C/0x0B soft-page dispatch in L006.
initech-cqej's decline/Bochs/clip test debt does not own the literal
Exit-versus-Quit routing failure in L005. Finder scrollbar follow-ups do not
implement Word's absent controller. P5 Face B and P6's shortfall ledger are
dependencies of several features, not proof of completed Face A commands.
Other dispatched lanes and the named SAMIR reds get no items from this report.

## All findings

| ID | Verdict | Owner | Note |
| --- | --- | --- | --- |
| L001 | LANE | initech-mcep | in lane initech-mcep (atomicsave). face_a.c:1249 truncates before :1252 write; :1256-1260 has no rollback. Prior ORIGINAL.WP5: 373 -> 0 bytes in both S02/S04. Confirmed D; no new item. |
| L002 | LANE | initech-mcep | in lane initech-mcep (atomicsave). The same face_a.c:1249-1260 failure path leaves new destinations in place; NEW/SMALL are zero bytes. Confirmed D; no new cleanup ticket. |
| L003 | D | B03 | Fixed 24-row geometry at face_a.c:186-206 and no resize adaptation at :2412 clip the existing status/prompt workflow. |
| L004 | D | B05 | The gate silently returns only the first 256 raw records (tbxgate.c:1078-1092); listfiles.c:143 drops two dot records, producing the observed 254 count. |
| L005 | D | B04 | Word installs Exit (face_a.c:244); tbx_ask_quit matches only Quit (tbxgate.c:1550). kmain.c:5720-5723 reports refusal only to serial. cqej decline-test debt is not this defect. |
| L006 | F | B10 | No soft-page single-byte decoder: piece.c:245-258 rejects 0x8C/0x0B. MINT39/MINT44 fail at 0x23E/0x2BB. Safe refusal is correct until support is built; the interoperability feature remains missing. |
| L007 | DUP | initech-8q6j | Underlying F: table rendering is unbuilt. piece.c:308 classifies table groups as generic codes; face_a.c:613-627 draws only text/spaces/tab/indent. P4 already owns WP tables. The report demonstrates lost display structure, not lost serialized table bytes. |
| L008 | F | B11 | Inline C1/E0 remains IW_U_CODE in piece.c:299-308; edit.c:388-421 only positions paragraph Justification, so no inline center consumer exists. |
| L009 | F | B11 | Inline C1/60 remains IW_U_CODE in piece.c:299-308; the separate paragraph Right Justification consumer does not implement this code. |
| L010 | DUP | initech-9m65 | Underlying F: extended-character rendering is already named in the P2 remainder. piece.c:308 treats C0 as generic code; face_a.c:613-627 skips it and codes.c:391-392 uses [?]. Preserve the C0 bytes while adding a glyph consumer. |
| L011 | DUP | initech-8q6j | Underlying F: D6 footnote rendering/numbering is absent from face_a.c:613-627 and codes.c:391-392. P4 explicitly owns footnotes/endnotes; coordinate the overlapping P2 research bead 9m65, do not create a reference-number-only duplicate. |
| L012 | D | B07 | event.c:539 sends m+Control, but face_a.c:2311 types m before :2313 checks ch==13. Existing intended SRt search branch is unreachable for the actual chord. |
| L013 | DUP | initech-naoz | The exact existing long-word Tab/Indent geometry owner. edit.c:63 advances to a stop beyond width; :389/:401 retains it as next-line indent, explaining one-character lines. Exact historical boundary rule remains unminted W-16; no invented clamp or refusal oracle. |
| L014 | DUP | initech-naoz | Same boundary owner as L013: edit.c:375 breaks only before text, while :1052 writes the next stop as absolute payload. The screen/payload inconsistency is reproducible; capture real WP W-16 before prescribing a coordinate. |
| L015 | D | B02 | name_key at face_a.c:1725 silently strips spaces before take_name can reject them; user-requested destination changes. |
| L016 | DOC | B50 | WRONG as a system defect: ref-l.tex:87 promises list Esc, but real WP LISTF ledger:126 says Esc does nothing. face_a.c:1526 matches WP. Correct the manual. |
| L017 | D | B06 | M_LOOK at face_a.c:1503 bypasses the shared F1 cancellation route. The implemented Look menu violates ref-c.tex:12-15; retain real Esc/Space/F7 behavior. |
| L018 | D | B06 | Existing text Look resets byte offset to zero at face_a.c:1614 and scroll input is WP-only at :1504, stranding all later text lines. |
| L019 | F | B06 | Next/Prev actions are unbuilt: face_a.c:1502-1517 handles neither 1 nor 2 although :1626 renders their bright menu. Deliver real navigation, with truthful availability during work. |
| L020 | F | B22 | No typed-directory editor: M_LISTDIR at face_a.c:1488-1491 only opens root on Enter. Supporting file paths elsewhere do not implement this entry point. |
| L021 | F | B22 | Default-directory action never built: face_a.c:1488-1491 ignores = and :1347-1350 explicitly admits it. The hint overstates availability; same directory-resolver owner as L020/L088. |
| L022 | F | B15 | No document hit test or mouseDown case in face_a.c:2412-2438. Classified as absent native input integration, not a malfunctioning hit-test routine. |
| L023 | F | B15 | No Word viewport/scroll controller is registered; kmain.c:2310-2315 deliberately ignores that bar. The gray-arrow shot does not establish an enabled broken control. Missing tenant integration remains F. |
| L024 | F | B12 | cursor_key at face_a.c:1952-1989 has no PgUp/PgDn route; existing Home screen/document movement is a separate command. |
| L025 | F | B12 | No Go To mode/dispatch exists at face_a.c:1952-1989/:2100-2116; Ctrl-Home degenerates into Home and a following digit reaches typing. |
| L026 | F | B13 | cursor_key at face_a.c:1984-1985 receives no modifier argument and implements character motion only. Word movement is unbuilt. |
| L027 | F | B13 | face_a.c:2108 always uses single-unit ED_DELETE; Ctrl-Delete has no word-deletion adapter. |
| L028 | F | B14 | face_a.c:1986 maps End to line-end navigation regardless of Control; a delete-to-line-end operation never exists at this entry point. |
| L029 | F | B14 | No Ctrl-PgDn route or page-delete confirmation state at face_a.c:2107-2110. A new adapter must use the existing layout/undo safely. |
| L030 | F | B16 | Insert/typeover state and handler are absent from face_a.c:2100-2116; only insert typing exists. |
| L031 | F | B17 | No Repeat Value state exists; Esc is ignored in edit_key and digits become text at face_a.c:2115. Home prefix state at :2369 is not a repeat engine. |
| L032 | F | B18 | block_key at face_a.c:2000-2019 never handles Enter. Paragraph selection support at Ctrl-F4 is not a Block+Enter adapter. |
| L033 | F | B19 | block_key at face_a.c:2000-2019 omits F10, while ordinary Save at :2070 is unreachable with Block active. Existing block serialization is only supporting machinery. |
| L034 | F | B20 | block_key at face_a.c:2000-2019 has no Shift-F3/case route; Switch is gray in spec/iword/fkeys.h:44. |
| L035 | F | B18 | block_key at face_a.c:2000-2019 omits F2/Shift-F2, blocking reuse of the otherwise implemented search engine. |
| L036 | DUP | initech-9m65 | Underlying F: Append is already explicitly named in the P2 remainder. face_a.c:1832-1839 handles only Move/Copy/Delete, so choice 4 returns without a file workflow. Same unbuilt Append mechanism, including active-block entry. |
| L037 | DUP | initech-9m65 | Underlying F: Tabular Column is explicitly named in the P2 remainder. face_a.c:1807-1810 accepts only Block (1); no column selection/move engine exists behind the other entry. |
| L038 | DUP | initech-9m65 | Underlying F: Rectangle is explicitly named in the P2 remainder. face_a.c:1807-1810 returns on choice 3; use the same pending rectangular-block mechanism rather than a new active-block ticket. |
| L039 | F | B17 | Home-Space action is absent: face_a.c:2369 consumes a Home prefix and :2115 inserts ordinary 0x20. Existing A0 scanning/layout does not implement its keyboard producer. |
| L040 | DUP | initech-9m65 | Underlying F: Compose/extended-character entry shares the named extended-character remainder. face_a.c:2115 types printable Ctrl-2/e/apostrophe events with no Compose state; pair with the C0 renderer in L010. |
| L041 | D | B09 | Accepted units are persisted at face_a.c:1172-1185 but ignored by status/entry consumers :733/:745/:926/:1849. An omitted consumer of an accepted setting is D despite documentation of the omission. |
| L042 | D | B01 | Accepted Yes flag is persisted at face_a.c:1172-1185, displayed at :1045 and ignored by write_file :1236-1263. D for the disconnected protection setting; atomic failed-save safety stays in mcep. |
| L043 | F | B21 | Timed Backup is gray at face_a.c:1044 and has no setup/timer consumer at :1167-1171/:2412. |
| L044 | DUP | initech-9m65 | Underlying F: Base Font screen is explicitly in the existing P2 title. face_a.c:1767-1773 implements only Font choices 1/2/3, not 4. Real MINT28 shot 25 already exists. |
| L045 | DUP | initech-9m65 | Underlying F: Print Color screen is explicitly in the existing P2 title. face_a.c:1767-1773 implements no choice 5. Real MINT28 shot 27 already exists. |
| L046 | D | B49 | face_a.c:786/:814 paints unavailable Font actions as normal text; real feature delivery is already on 9m65. This batch owns only truthful availability feedback. |
| L047 | D | B49 | paint_format at face_a.c:937-964 paints inactive groups/settings normally, while :1881-1890 ignores their keys. Separate feedback defect from unbuilt settings. |
| L048 | F | B26 | Display is gray at face_a.c:1030 and setup_key :1167 never accepts choice 2. |
| L049 | F | B27 | Initial Settings is gray at face_a.c:1031 and setup_key :1167 has no choice 4 or default-document consumer. |
| L050 | F | B28 | Keyboard Layout is gray at face_a.c:1031 and setup_key :1167 has no choice 5; keys still use fixed IW_FKEYS. |
| L051 | F | B29 | Location of Files is gray at face_a.c:1032 and setup_key :1167 has no choice 6 or per-class path resolver. |
| L052 | F | B30 | Format dispatch at face_a.c:1881-1884 only enters Line; Page is unbuilt. No child control was observed or individually alleged defective. |
| L053 | F | B31 | Format dispatch at face_a.c:1881-1884 has no Document route; unobserved child settings remain feature scope, not invented findings. |
| L054 | F | B32 | Format dispatch at face_a.c:1881-1884 has no Other route; unobserved child settings remain feature scope. |
| L055 | F | B33 | Line choice 1 omitted at face_a.c:1885-1890; :946 paints a fixed hyphenation value. Share the break-policy implementation with L056. |
| L056 | F | B33 | Line choice 2 omitted at face_a.c:1885-1890; :947-948 paints fixed zone values. Same missing policy consumer as L055. |
| L057 | F | B34 | Line choice 4 omitted at face_a.c:1885-1890 and :950 paints fixed height. Implemented Line Spacing is a distinct setting. |
| L058 | F | B35 | Line choice 5 omitted at face_a.c:1885-1890 and :951 paints a fixed numbering value; no numbering state is consumed. |
| L059 | F | B37 | Line choice 8 omitted at face_a.c:1885-1890 and :963 paints default stops; edit.c:63 hardcodes five-column stops. Share custom stop model with L081. |
| L060 | F | B36 | Line choice 9 omitted at face_a.c:1885-1890 and :964 paints fixed protection state; no paragraph-aware pagination policy exists. |
| L061 | D | B08 | parse_inches at face_a.c:1849 returns 0 for valid zero and invalid input; :1924 suppresses both. Core margins at edit.c:1271 accept zero when width is valid. |
| L062 | F | B38 | Help is explicitly IW_FK_gray at spec/iword/fkeys.h:30; face_a.c:2086 only shows its name. No help route/topic engine. |
| L063 | F | B39 | End Field is explicitly IW_FK_gray at spec/iword/fkeys.h:36; face_a.c:2086 has no merge-field operation. |
| L064 | F | B20 | Switch is explicitly IW_FK_gray at spec/iword/fkeys.h:44; face_a.c:2086 has no secondary-document route. Share context-sensitive command delivery with block case L034. |
| L065 | F | B40 | Reverse Indent is gray at spec/iword/fkeys.h:45; face_a.c:2066 only implements forward Indent. |
| L066 | F | B41 | Date/Outline is gray at spec/iword/fkeys.h:46; face_a.c:2086 has no dated-code or outline-menu route. |
| L067 | F | B11 | Center is gray at spec/iword/fkeys.h:47; no keyboard producer or inline-code layout consumer. Share the C1 alignment implementation with imported L008/L009 and L072. |
| L068 | DUP | initech-8q6j | Underlying F: spec/iword/fkeys.h:48 grays Print; face_a.c:2086 only names it. P4 already owns print-to-PRN; include List Files Print (gray key 4) in that pickup, with a real output stream, not another placeholder. |
| L069 | F | B39 | Merge Codes is gray at spec/iword/fkeys.h:50; face_a.c:2086 provides no merge code/input state. Shared merge subsystem with L063/L083. |
| L070 | DUP | initech-xp4k | Underlying F: spec/iword/fkeys.h:56 grays Thesaurus; face_a.c:2086 has no data consumer. The existing P5 title explicitly owns the spell-check/thesaurus data strategy; retain actual lookup delivery there. |
| L071 | F | B42 | Mark Text is gray at spec/iword/fkeys.h:60; face_a.c:2086 has no marking code/consumer route. |
| L072 | F | B11 | Flush Right is gray at spec/iword/fkeys.h:61; no inline-code insertion/layout route. Same C1 alignment owner as L008/L009/L067. |
| L073 | DUP | initech-8q6j | Underlying F: spec/iword/fkeys.h:62 grays Columns/Table and face_a.c:2086 has no editor. P4 already owns columns and WP tables, including shared imported structure in L007. |
| L074 | F | B43 | Style is gray at spec/iword/fkeys.h:63; face_a.c:2086 has no WP style definitions/command. P5 Face B integration is related, not the same absent entry point. |
| L075 | F | B44 | Graphics is gray at spec/iword/fkeys.h:64; face_a.c:2086 has no resource/placement renderer or graphics command. |
| L076 | F | B45 | Macro playback is gray at spec/iword/fkeys.h:65; face_a.c:2086 has no playback engine. Same storage/command interpreter as L084. |
| L077 | F | B46 | Shell is gray at spec/iword/fkeys.h:70; face_a.c:2086 has no child-command handoff/return contract. |
| L078 | DUP | initech-xp4k | Underlying F: spec/iword/fkeys.h:71 grays Spell; face_a.c:2086 has no dictionary consumer. Existing P5 owns the spell-check strategy and reduced-list implementation. |
| L079 | F | B47 | Screen is gray at spec/iword/fkeys.h:72; face_a.c:2086 has no Rewrite/split command. Existing Reveal split does not implement arbitrary editing views. |
| L080 | F | B48 | Text In/Out is gray at spec/iword/fkeys.h:74; face_a.c:2086 has no converter UI. wp5.c:110 is only a supporting lossy extractor. |
| L081 | F | B37 | Tab Align is gray at spec/iword/fkeys.h:75; no aligned-tab producer/consumer. Share persisted stops and geometry with Line Tab Set L059. |
| L082 | DUP | initech-8q6j | Underlying F: spec/iword/fkeys.h:76 grays Footnote; face_a.c:2086 has no note editor. P4 footnotes/endnotes owns creation, reference rendering (L011) and layout; 9m65 is related research only. |
| L083 | F | B39 | Merge/Sort is gray at spec/iword/fkeys.h:78; face_a.c:2086 has no execution/sort module. Shared merge subsystem with L063/L069. |
| L084 | F | B45 | Macro Define is gray at spec/iword/fkeys.h:79; face_a.c:2086 has no recorder. Same unbuilt macro subsystem as L076. |
| L085 | F | B23 | List key 2 live=0 in spec/iword/listfiles.h:117; face_a.c:1521-1532 never dispatches Delete. Add protection/confirmation to the new file adapter. |
| L086 | F | B23 | List key 3 live=0 in spec/iword/listfiles.h:117; face_a.c:1521-1532 never dispatches Move/Rename. Share selected-file path/error handling with L085/L089. |
| L087 | F | B24 | List key 5 live=0 in spec/iword/listfiles.h:118; face_a.c:1521-1532 never changes display mode. |
| L088 | F | B22 | List key 7 live=0 in spec/iword/listfiles.h:120; face_a.c:1521-1532 has no Other Directory route. Same resolver as F5 typed/default paths L020/L021. |
| L089 | F | B23 | List key 8 live=0 in spec/iword/listfiles.h:120; face_a.c:1521-1532 has no Copy route. Safe output mechanics depend on mcep, not a new atomic-save ticket. |
| L090 | F | B25 | List key 9 live=0 in spec/iword/listfiles.h:120; face_a.c:1521-1532 has no Find query/results route. |
| L091 | F | B25 | List key N live=0 in spec/iword/listfiles.h:121; face_a.c:1521-1532 has no Name Search state. Share the listing/query model with L090 while preserving distinct semantics. |

## Developer batches

Ordered by user harm: recovery/data destination protection, blocked existing
workflows, missing features, then availability/manual polish. Each batch has
one mechanism/entry owner. A large unbuilt submenu/engine has a bounded first
pickup; it stays open for the remaining named feature rather than closing on
a gray menu or a success-only placeholder. All user-visible changes carry the
corresponding manual/key-strip delta and explicit locked-spec changes where
necessary. No independent golden may be generated from the implementation.
Every stated oracle is required acceptance work, NOT a result from this triage.
Window features require a locked target trace and record-flair clip (Rule 14);
QEMU/Bochs coverage applies to new integration paths, with real WP/86Box or
supplied real mints as the compatibility reference. Keep existing gate vectors
and mutants; add the named regression mutant rather than weakening a golden.


### B01 - IWORD: honor Original Document Backup before replacing a saved version (P2, bug)

Covers: L042.

Root: os/iword/app/face_a.c:1172-1185 persists setting byte 6, but write_file at :1236-1263 never consumes it; :1045 only displays it.

Ground truth: pubs/manuals/initechword-reference/ref-s.tex:308-315 admits the inert setting; WP Reference Backup Options pp. 42-43 and the existing Yes preference define the recovery intent. The admission is not deliberate canon.

Pickup: Keep the preceding successfully saved document as a recoverable original when Yes is selected. Coordinate with initech-mcep for atomic replacement; do not reimplement or refile its failed-save rollback. Capture real WP backup naming/rotation before choosing bytes or names.

Required oracle: Extend harness/proptest/test_iword_face.c with Yes/No, second/third save, backup-name collision and disk-full/close/rename fault cases. Extract original and new versions with mtools and read both independently/with real WP. Mutant: ignore byte 6. Replay S01 backup keys and require a record-flair clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-backup-yes.png; docs/audits/2026-10-23-iword/evidence/s01-directory.txt.


### B02 - IWORD: reject invalid filename spaces without silently changing the destination (P2, bug)

Covers: L015.

Root: os/iword/app/face_a.c:1725 accepts only ch > space, so take_name at :1202 never sees spaces and can approve a different name.

Ground truth: pubs/manuals/initechword-reference/ref-f.tex:16-37 specifies the allowed characters and an Invalid file name refusal.

Pickup: Retain invalid input for validation or visibly refuse it at input; never silently turn BAD NAME.WP5 into BADNAME.WP5. Apply to Save and Retrieve, including offered-name replacement.

Required oracle: Extend test-iword-face with leading/internal/trailing spaces, legal names, backspace correction and existing-destination sentinels. Emulator Save/Retrieve must show the refusal and preserve disk hashes. Mutant: restore the ch > space filter. Record the trace/clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-space-name.png; docs/audits/2026-10-23-iword/evidence/s01-extracted/BADNAME.WP5.


### B03 - IWORD: keep status and modal prompts reachable after window resize (P2, bug)

Covers: L003.

Root: os/iword/app/face_a.c:186-206 fixes 76x24 cells and status/pane rows; :843 and :2412 repaint these constants after resize without reading content geometry.

Ground truth: PRD 6.3, ADR-0013 native document windows, and the audited grow-control workflow; exact compact layout is an authored native adaptation, not a claimed WP DOS screen.

Pickup: Use actual content bounds for viewport, status, menus and Reveal layout, or enforce a truthful supported minimum size. A grow operation must not hide the only Save/error/cancel prompt.

Required oracle: Extend test-iword-face geometry mock and test-flair-iword with shrink/grow during editing, Save, error, Setup and Reveal; assert prompt pixels remain visible and cancel preserves bytes. Mutant: retain LINE_STATUS=23 at the small size. Locked QEMU/Bochs trace plus record-flair clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-resize.png.


### B04 - FLAIR/IWORD: deliver Finder power requests to Word Exit and expose the save decision (P2, bug)

Covers: L005.

Root: os/flair/tbxgate.c:1550 recognizes only an enabled item literally named Quit; os/iword/app/face_a.c:244 names it Exit. os/milton/kmain.c:5715-5723 cancels on -1 with serial-only cannot-ask.

Ground truth: Inside Macintosh VI Quit Application semantics cited at os/milton/kmain.c:5691-5705; Word request_exit at os/iword/app/face_a.c:1703-1707 is the existing save guard.

Pickup: Represent the quit action independently of its localized/period label and bring Word forward for a usable save/cancel interaction. Preserve dirty text on refusal and visibly explain a tenant that cannot answer. initech-cqej covers decline/Bochs test debt, not this Exit-versus-Quit dispatch defect.

Required oracle: Extend the Toolbox quit-route host oracle with Exit, Quit, absent and disabled actions. Drive Word dirty/clean/save/no/cancel/failed-save through Finder Restart and Shut Down; require responsive prompts, preserved bytes on cancel and QEMU/Bochs agreement. Mutant: require literal Quit. Record the clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-before-shutdown.png.


### B05 - Toolbox/IWORD: detect or eliminate silent directory enumeration truncation (P2, bug)

Covers: L004.

Root: os/iword/app/face_a.c:1369 and :1412 request only 256 records. os/flair/tbxgate.c:1078-1092 keeps walking but stops copying/counting at capacity and returns success; os/iword/core/listfiles.c:143 then removes dot entries, leaving 254 files.

Ground truth: pubs/manuals/initechword-reference/ref-l.tex:73-99 promises file listing/count/used size; prepare_stress.py and stress-before-directory.txt independently establish 270 files and 92340 bytes.

Pickup: Make overflow explicit in the gate contract and app, with continuation/paging or an honest visible partial-list warning. Do not fix this by only increasing LF_MAX: root directories, subdirectories and counts must have a defined capacity contract.

Required oracle: Add gate and test_iword_listfiles.c cases for 254/255/256/270+ files, dot entries and subdirectories. Emulator must reach F269 or visibly disclose the limit and partial totals; compare against mdir. Mutant: suppress overflow/continuation. Lock the trace and record-flair clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-many-first.png; docs/audits/2026-10-23-iword/evidence/stress-before-directory.txt.


### B06 - IWORD Look: restore cancellation, text scrolling and adjacent-document navigation (P2, bug)

Covers: L017,L018,L019.

Root: os/iword/app/face_a.c:1502-1517 omits F1 and Next/Prev handlers and gates motion on g_look_wp; :1614 resets plain-text rendering to byte zero every paint.

Ground truth: pubs/manuals/initechword-reference/ref-c.tex:12-15 (Cancel), WP Reference Look printed p. 363/PDF pp. 381-384, and spec/iword/listfiles.h:137 (Look menu).

Pickup: Use one read-only Look viewport with WP/text scroll state and real Next/Prev document selection. F1 must cancel without editing the active document. Keep directory selection and current unsaved document intact; define missing/read-error and end-of-list behavior from a real WP mint.

Required oracle: Extend harness/proptest/test_iword_face.c and test_iword_listfiles.c with 80-line text, WP files, F1/Esc/F7/Space and Next/Prev boundaries. Emulator must reach text line 080 and another named file, then restore the unchanged editing document. Mutants: force text offset zero, omit F1, ignore Next. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-look-cancel-f1.png; docs/audits/2026-10-23-iword/shots/s03-look-text-top.png; docs/audits/2026-10-23-iword/evidence/stress-fixtures/TEXTLONG.TXT; docs/audits/2026-10-23-iword/shots/s03-look-long-before-next.png.


### B07 - IWORD Search: translate Ctrl-M from the actual FLAIR key event (P2, bug)

Covers: L012.

Root: os/flair/event.c:539-547 supplies printable m plus Control; os/iword/app/face_a.c:2311 accepts printable text before :2313 tests ch==13 for Ctrl-M, so that existing SRt branch is unreachable for the real chord.

Ground truth: WP Reference printed p. 573 (PDF p. 591), independently re-extracted: Ctrl-m includes SRt. The audit evidence/references/wp-search.txt excerpt is from earlier pages and does not contain this instruction.

Pickup: Recognize the Control modifier plus M virtual/character key before printable query insertion; insert one 0x0D search token. Keep Ctrl-Enter hard-page and ordinary Enter hard-return behavior distinct.

Required oracle: Extend test_iword_p3.c/search tests using the exact 0x326D+Control event, then replay target Ctrl-M search across a wrapped line in both directions. Assert query bytes/label and found range. Mutant: require ch==13 again. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-search-srt-query.png.


### B08 - IWORD: distinguish zero margins from invalid numeric input (P2, bug)

Covers: L061.

Root: os/iword/app/face_a.c:1849-1863 returns zero for both valid 0 and invalid input; :1924 updates the margin only when v is nonzero. os/iword/core/edit.c:1271 permits zero with valid remaining width.

Ground truth: pubs/manuals/initechword-reference/ref-m.tex:64-66 accepts readable margins below 9 inches; the core range contract accepts zero.

Pickup: Return parse success separately from the value. Apply zero when valid; keep blank/default/cancel separate and visibly refuse values that violate paper width. Preserve existing document/cursor on refusal.

Required oracle: test-iword-face plus core tests for 0, 0.00, blank, malformed, valid fractions and impossible left+right combinations; decode D0/01 values independently after emulator Save. Mutant: use if(v). Record format trace/clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-zero-margins.png.


### B09 - IWORD: consume the selected entry and status measurement units (P2, bug)

Covers: L041.

Root: os/iword/app/face_a.c:1172-1185 stores unit bytes 4/5, but status :733/:745, format :926 and parse_inches :1849 always use inches; no unit consumer exists outside the Setup labels.

Ground truth: spec/iword/settings.h:5-8 defines independent entry/status unit preferences; WP Reference Units of Measure pp. 698-700; ref-s.tex:360-362 documents the incomplete consumers.

Pickup: Implement both preference consumers with explicit units and round-trip conversion for i/c/p/w/u. Keep canonical stored WP coordinates in WPU and settings bytes unchanged; do not merely rename inch values.

Required oracle: Extend test_iword_p3_part2.c and test-iword-face with independent rational conversion fixtures and differing entry/status settings; emulator save/relaunch, margins and Pos/Ln must use the chosen units. Mutant: force inches at one consumer. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-units-cm.png.


### B10 - IWORD: support the verified soft-page code variants in real multipage WP5 files (P2, feature)

Covers: L006.

Root: os/iword/core/piece.c:245-258 admits neither 0x8C nor 0x0B as a single unit; os/iword/fs/wp5.c:54-56 rejects the file. Read-only byte inspection finds MINT39 first rejected at 0x23E (0x8C), MINT44 at 0x2BB (0x0B).

Ground truth: ../wordperfect51-decomp/mint/wpMINT39.steps and wpMINT44.steps, their real saved files/captures, and the audit independent decoder. Existing locked single-code data predates these mints.

Pickup: Add independently verified HRt-SPg and SRt-SPg semantics to unit scanning, paging, display, editing and serialization. Record the locked-spec update. Keep refusal for unknown/malformed codes. initech-tu3d covers prefix packets/D4 research, not the missing single-byte dispatch.

Required oracle: Extend test-iword-core and test-iword-wp5-roundtrip with MINT39/44 and mutation-proven soft-page type/round-trip checks. Emulator must retrieve, navigate pages, Save and reopen in real WP with text/paragraphs preserved. Mutant: reject or flatten one soft-page variant. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-import-mint39.png; docs/audits/2026-10-23-iword/evidence/fixtures/MINT39.WP5.


### B11 - IWORD: implement inline Center and Flush Right codes and their entry commands (P2, feature)

Covers: L008,L009,L067,L072.

Root: os/iword/core/piece.c:299 recognizes only C1/02 as positioned content; C1/E0 and C1/60 fall back to zero-width IW_U_CODE. os/iword/core/edit.c:388-421 handles paragraph Justification only. spec/iword/fkeys.h:47/:61 gray the two insertion commands.

Ground truth: ../wordperfect51-decomp/goldens/minted/MINT01.shots/02_typed.png and MINT03.shots/02_typed.png; spec/iword/wp5_format.h and codes.c:343-344 identify the inline codes. ref-c.tex:118-124 acknowledges left-margin fallback.

Pickup: Share the inline alignment layout model between imports and Shift-F6/Alt-F6 insertion, including end-of-alignment 0x83, wrapping, cursor and Reveal. Preserve the existing separate Format Justification implementation.

Required oracle: Extend test-iword-face/codes/WP5 differential against real MINT01/03 positions and newly minted command traces; verify display coordinates and round-trip bytes. Mutants: treat C1/E0 or C1/60 as zero-width formatting only. Record emulator clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-real-mint01.png; docs/audits/2026-10-23-iword/evidence/fixtures/MINT01.WP5; docs/audits/2026-10-23-iword/shots/s01-import-mint03.png; docs/audits/2026-10-23-iword/evidence/fixtures/MINT03.WP5; docs/audits/2026-10-23-iword/shots/s01-key-center.png; docs/audits/2026-10-23-iword/shots/s01-key-flush-right.png.


### B12 - IWORD: add page navigation and a real Go To prompt (P2, feature)

Covers: L024,L025.

Root: os/iword/app/face_a.c:1952-1989 has no PgUp/PgDn or Ctrl-Home dispatch; :2108-2116 falls back to ordinary cursor/typing and never enters a Go To mode.

Ground truth: WP Reference Cursor Movement p. 104 and Go To p. 224, cited in audit evidence/references/wp-cursor.txt and wp-goto.txt; confirm exact page behavior against the original manual/mint.

Pickup: Use the document page model to reach next/previous page and parse Ctrl-Home page numbers. No navigation prefix or its digits may enter document text; cancellation preserves cursor/document. Coordinate soft-page support with B10.

Required oracle: Extend test-iword-face/core with hard/soft/mixed pages, first/last boundaries, invalid page and Go To cancel. Emulator long-document trace must assert page/cursor and unchanged saved bytes. Mutants: ignore PgDn or route Go To digits to typing. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-long-open.png; docs/audits/2026-10-23-iword/shots/s01-goto-six.png.


### B13 - IWORD: share word boundaries for Ctrl-arrow movement and Delete Word (P2, feature)

Covers: L026,L027.

Root: os/iword/app/face_a.c:1984-1985 ignores modifiers in cursor_key; :2108 always invokes ED_DELETE for Delete, with no word-operation route.

Ground truth: WP Reference pp. 104 (Word Right) and 116 (Ctrl-Del); ref-c.tex cursor limits and ref-d.tex:57-59 admit the reduced routes.

Pickup: Implement modifier-aware word movement/deletion over complete document units. Establish punctuation, whitespace, attributes and hard-return boundaries from real WP, then share the boundary walker and existing undo machinery.

Required oracle: Host table-driven real-WP boundary corpus plus test-iword-face exact modifier events; emulator one two three and coded-word controls must move/delete whole words and undo accurately. Mutant: use one-unit movement/deletion. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-ctrl-right.png; docs/audits/2026-10-23-iword/shots/s03-delete-word.png.


### B14 - IWORD: add confirmed delete-to-line and delete-to-page commands (P2, feature)

Covers: L028,L029.

Root: os/iword/app/face_a.c:1986 routes End to navigation regardless of Control; :2107-2110 has no Ctrl-End/Ctrl-PgDn deletion or page confirmation state.

Ground truth: WP Reference Delete p. 116: Ctrl-End deletion and Ctrl-PgDn then Y, independently checked against the original manual before locking edge transcripts.

Pickup: Connect deletion extents to the shared layout/complete-unit and undo contracts. Page deletion must wait for the real confirmation; No/F1/EOF-style input failure preserves bytes. Do not silently turn End navigation itself into deletion.

Required oracle: Host line/page boundary cases with codes, soft wraps and empty final pages; emulator confirmed/cancelled delete plus Undelete and extracted WP bytes. Mutants: navigation-only Ctrl-End and unconditional page delete. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-delete-end-line.png; docs/audits/2026-10-23-iword/shots/s05-delete-page-at-start.png.


### B15 - IWORD: connect document hit testing and scrollbars to its viewport (P2, feature)

Covers: L022,L023.

Root: os/iword/app/face_a.c:2412-2438 has no content mouse/scroll case or viewport-control registration. os/milton/kmain.c:2310-2315 ignores a bar with no active scroll controller.

Ground truth: WP Reference Mouse Support and PRD 6.3/ADR-0013 native document interaction. Scrollbar movement is a native adaptation; the shot shows gray arrows, not proof of an enabled controller.

Pickup: Expose needed Toolbox events/state and map clicks/scroll steps through actual resized viewport, formatting and Reveal geometry. A click must place the cursor at the clicked unit; active scrollbar state must track document extent. Coordinate geometry B03.

Required oracle: Host point-to-unit/viewport boundary oracle independent of the renderer; emulator click line 1 while cursor is on line 23, then arrow/page/thumb scrolling, resize and Reveal. Mutants: ignore content mouseDown or leave scroll state unregistered. QEMU/Bochs and record-flair clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-home-down.png.


### B16 - IWORD: implement Insert/typeover as an editor mode (P2, feature)

Covers: L030.

Root: os/iword/app/face_a.c:2100-2116 contains only insert typing; VK Insert has no handler and the app keeps no typeover state.

Ground truth: pubs/manuals/initechword-reference/ref-i.tex:73-74 admits Ins is inert; lock the WP 5.1 Insert/typeover rules from the original reference and a real mint before implementing code boundaries.

Pickup: Toggle insert/typeover, show its state and replace the appropriate next text unit without splitting formatting codes. Preserve documented end-of-line/end-of-file and undo behavior.

Required oracle: Extend core/face host tests with insertion/typeover toggles, codes, returns and EOF; emulator Insert Z over existing text then undo/retrieve. Mutant: always insert. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-typeover.png.


### B17 - IWORD: implement Repeat Value and Home-Space prefix actions (P2, feature)

Covers: L031,L039.

Root: os/iword/app/face_a.c:2369-2370 retains only a Home count, and :2115 types Space normally; Esc enters no repeat-count state. The existing Home prefix is consumed without a Space action.

Ground truth: WP Reference Cursor Movement p. 105 (Repeat Value); ../wordperfect51-decomp/mint/wpMINT05.steps and specs/file-formats/wp5-body-codes.md section 2 verify Home-Space emits A0.

Pickup: Add bounded explicit prefix states so counts do not become document text and Home-Space inserts one hard-space unit. Capture repeat scope, cancellation and overflow rules from real WP; reuse existing hard-space layout support.

Required oracle: Host prefix state tables and exact key events; emulator Esc 3 Right moves three units without inserting 3, Home-Space Save decodes A0 and prevents a wrap there. Mutants: type count digits or emit 0x20. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-repeat-value.png; docs/audits/2026-10-23-iword/shots/s03-hardspace-compose.png; docs/audits/2026-10-23-iword/evidence/s03-extracted/CHARS.WP5.


### B18 - IWORD Block: connect paragraph extension and code-aware Search (P2, feature)

Covers: L032,L035.

Root: os/iword/app/face_a.c:2000-2019 block_key handles arrows/attributes/move/delete but omits Enter and F2/Shift-F2. Paragraph selection and search exist at other entry points, not this one.

Ground truth: WP Reference Block pp. 49-50 and Search p. 573; original manual extraction confirms Enter extends to HRt and Search expands/contracts an active block.

Pickup: Retain the anchor while Enter extends to paragraph end and Search moves the selected endpoint. Match forward/backward, cancellation and not-found behavior without running replacement over a stale block.

Required oracle: Extend test-iword-face and test_iword_p3.c with forward/reverse block, next HRt, code boundaries and successful/failed/cancelled searches. Emulator checks highlight and saved bytes before/after action. Mutants: drop Enter or clear the anchor on Search. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-block-enter-at-start.png; docs/audits/2026-10-23-iword/shots/s03-block-search.png.


### B19 - IWORD Block: save the selected stream as an independent WP5 file (P2, feature)

Covers: L033.

Root: os/iword/app/face_a.c:2000-2019 returns from block_key without dispatching F10; the Save route at :2070 is reachable only without an active block.

Ground truth: WP Reference Block pp. 49-51; ref-b.tex:50-60 acknowledges the limited block commands; WP5 prefix/unit contracts in spec/iword/wp5_format.h.

Pickup: Use the existing self-contained block stream with a valid independent prefix; do not compact/adopt the entire editing document as though it was saved. Integrate safe new/replace writes through initech-mcep.

Required oracle: Host selected text/attribute/return cases and independent WP5 decoding; emulator Block Save, cancel, overwrite No and failed save must preserve source and previous destination, while real WP opens only the selected output. Mutant: save entire document. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-block-save.png.


### B20 - IWORD Switch: add secondary-document switching and block case conversion (P2, feature)

Covers: L034,L064.

Root: spec/iword/fkeys.h:44 maps Switch to gray; os/iword/app/face_a.c:2013/:2019 provides no block Shift-F3 route and stores just one global document/editor.

Ground truth: WP Reference p. 57 for block Uppercase/Lowercase; docs/plans/INITECH-WORD-plan.md:173 names Doc 1/Doc 2 Switch as SHOULD, and spec/iword/fkeys.json:22 locks the key.

Pickup: Implement the context-sensitive Switch command: case conversion for a block, otherwise independently retained document contexts. Start with the case-conversion slice and retain secondary-document work on this same owner until complete; no one-window app or Face B claim closes it.

Required oracle: Host mixed-case/code-aware selected streams and two dirty contexts with separate cursor/undo/name state; emulator switch/save/cancel/exit both documents and block-case controls. Mutants: ignore Shift-F3 or share one document state. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-block-case.png; docs/audits/2026-10-23-iword/shots/s01-key-switch.png.


### B21 - IWORD: add timed document recovery through the save lifecycle (P2, feature)

Covers: L043.

Root: os/iword/app/face_a.c:1044 draws Timed Backup gray and setup_key :1167-1171 handles only Original Backup; :2412 has no timed recovery consumer.

Ground truth: WP Reference Backup Options pp. 42-43 and ref-s.tex:308-310; exact interval, recovery-file naming and recovery prompt require a real WP reference/mint.

Pickup: Add an injectable cooperative timer and a recoverable backup stream, without clearing the dirty flag or overwriting the saved document. Coordinate the atomic file mechanism on initech-mcep; original-version rotation remains B01.

Required oracle: Host fixed-clock trigger/reset and write/close/no-space failures; emulator edit/wait/relaunch/recover with extracted original/recovery bytes. Mutants: skip timer or mark dirty document clean after backup. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-timed-backup.png.


### B22 - IWORD List Files: implement typed, default and Other Directory paths (P2, feature)

Covers: L020,L021,L088.

Root: os/iword/app/face_a.c:1488-1491 accepts only Enter/cancel and always opens root; :1521-1532 has no key 7 action. :1347-1350 explicitly says typing and = are ignored despite the drawn hint.

Ground truth: WP Reference List Files PDF pp. 364-370; ../wordperfect51-decomp/mint/work/LISTF/mint-results-005.md; spec/iword/listfiles.h path/default hint and Other Directory choice.

Pickup: Use one validated directory/default resolver for F5 input, = and Other Directory. Retain current document/list selection on bad path/cancel and respect the actual supported mounted-drive contract.

Required oracle: Host path/default persistence/navigation cases; emulator F5 APPS, = default directory, key 7, parent/root, missing directory and cancel. Mutant: force root after every prompt. Compare mdir and record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-list-path-typing.png; docs/audits/2026-10-23-iword/shots/s01-list-default-dir.png; docs/audits/2026-10-23-iword/shots/s05-list-file-selected.png; docs/audits/2026-10-23-iword/evidence/s05-file-ops.json.


### B23 - IWORD List Files: add safe Delete, Move/Rename and Copy operations (P2, feature)

Covers: L085,L086,L089.

Root: spec/iword/listfiles.h:116-122 sets keys 2/3/8 live=0; os/iword/app/face_a.c:1521-1532 dispatches only Retrieve/Look/directory motion.

Ground truth: WP Reference List Files PDF pp. 364-370; spec/iword/listfiles.h menu records; ref-l.tex:103-107 admits these disabled operations.

Pickup: Build a shared selected-file operation adapter with confirmation, collision/read-only/no-space feedback and refreshed listing. Keep source and destination safe; use existing Toolbox file verbs and the atomic mechanism on initech-mcep rather than a second truncate/copy loop.

Required oracle: Mock file-operation host tests plus emulator disposable fixtures: cancel/confirm Delete, rename collision and copy success/failure, protected files and directory selection. Compare mtools hashes/FAT and unchanged active document. Mutants: wrong selected path, skip protection or truncate destination before copy. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s05-list-file-selected.png; docs/audits/2026-10-23-iword/evidence/s05-file-ops.json.


### B24 - IWORD List Files: add Short/Long Display mode (P3, feature)

Covers: L087.

Root: spec/iword/listfiles.h:118 sets key 5 inactive; os/iword/app/face_a.c:1521-1532 has no display-mode state or dispatch.

Ground truth: WP Reference List Files PDF pp. 364-370 and the real LISTF mint; ref-l.tex:103-107 acknowledges the missing mode.

Pickup: Implement the independently captured short/long listing layouts with selection and viewport preservation; adapt the real screen to native bounds explicitly.

Required oracle: Extend test_iword_listfiles.c with independent row goldens and selection mapping; emulator toggle at a scrolled list and retrieve the same selected file. Mutant: keep one layout or shift the selected identity. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s05-list-file-selected.png; docs/audits/2026-10-23-iword/evidence/s05-file-ops.json.


### B25 - IWORD List Files: implement Find and Name Search over the directory model (P2, feature)

Covers: L090,L091.

Root: spec/iword/listfiles.h:121-122 sets keys 9/N inactive; os/iword/app/face_a.c:1521-1532 never enters either query state.

Ground truth: WP Reference List Files PDF pp. 364-370 and a real WP mint for each distinct Find/Name Search rule; do not infer the two operations from their labels alone.

Pickup: Add distinct validated queries, results/no-match feedback and cancellation over a complete/explicitly limited listing (B05). Preserve selected identity, current directory and dirty editing document.

Required oracle: Host independently minted query/match/order fixtures; emulator positive/no-match/cancel cases, with a match after record 254 and both keys. Mutants: match all files or search only the first display page. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s05-list-file-selected.png; docs/audits/2026-10-23-iword/evidence/s05-file-ops.json.


### B26 - IWORD Setup: add the Display submenu and one verified display consumer (P3, feature)

Covers: L048.

Root: os/iword/app/face_a.c:1030 draws Display gray; :1167 accepts only Setup choice 3, so no Display state exists.

Ground truth: WP Reference Setup/Display and ref-s.tex:295-297; original WP reference and real capture must define each supported display setting.

Pickup: First pickup: capture the real Display menu and implement one useful native display option end-to-end, including persistence and cancellation. Track remaining options on this feature; merely opening a menu does not finish it.

Required oracle: Extend test_iword_p3_part2.c and test-iword-face with real independent menu/value goldens, changed renderer behavior and reload. Emulator Setup 2, apply/cancel and return to document; mutant: store value but ignore renderer. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-setup-2.png.


### B27 - IWORD Setup: add initial document settings and their creation-time consumer (P3, feature)

Covers: L049.

Root: os/iword/app/face_a.c:1031 draws Initial Settings gray; :1167 provides no choice-4 state. doc_reset is not connected to a saved initial-settings record.

Ground truth: WP Reference Initial Settings/Setup, spec/iword/settings.h current eight-byte record, and ref-s.tex:295-297.

Pickup: First pickup: mint the original menu and implement one verified document default consumed at New, without retroactively changing the open document. Version any extended settings contract explicitly; retain remaining options on the same owner.

Required oracle: Host default/new/existing document and settings-reload tests; emulator Setup 4, change default, clear/relaunch/new and compare saved WP codes. Mutant: change label only or apply to the existing document. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-setup-4.png.


### B28 - IWORD Setup: implement keyboard layout selection through actual key dispatch (P3, feature)

Covers: L050.

Root: os/iword/app/face_a.c:1031 draws Keyboard Layout gray; :1167 has no choice-5 state and function_key :2056 uses the fixed IW_FKEYS table.

Ground truth: WP Reference Keyboard Layout/Setup and spec/iword/fkeys.json; ref-s.tex:295-297 admits the missing setup route.

Pickup: First pickup: capture one supported native keyboard mapping and implement select/reset with a real dispatch consumer and a recovery/cancel key. Keep unimplemented layout/macro capabilities explicit on this feature.

Required oracle: Host mapping/default/reload and reserved-key cases; emulator Setup 5 then demonstrate a changed binding and reset without altering text on cancel. Mutant: persist selection but keep fixed dispatch. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-setup-5.png.


### B29 - IWORD Setup: connect Location of Files to a shared path resolver (P2, feature)

Covers: L051.

Root: os/iword/app/face_a.c:1032 draws Location of Files gray; :1167 never handles choice 6. take_name :1202 and settings_load use direct root-relative paths.

Ground truth: WP Reference Location of Files/Setup; ref-s.tex:295-297; ADR-0013 Toolbox file path contract, coordinated with List Files paths in B22.

Pickup: First pickup: capture the menu and add one verified file-location class with persistent validated paths and a real open/save consumer. Respect mounted volumes, explicit paths and missing-location feedback; do not claim every class from one working row.

Required oracle: Host resolver/search-order/cancel/reload tests and emulator files in two directories; verify the configured consumer reads the intended file. Mutant: always resolve from root. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-setup-6.png.


### B30 - IWORD Format: build the Page submenu over the document page model (P2, feature)

Covers: L052.

Root: os/iword/app/face_a.c:1881-1884 only dispatches Format choice 1 (Line); choice 2 never enters a Page state.

Ground truth: ../wordperfect51-decomp/mint/shots_wpMINT21/01_format_menu.png, WP Reference Format Page, and ref-f.tex:130-141. Child controls were not audited.

Pickup: First pickup: acquire the real Page submenu and implement one bounded page setting with serialized document behavior. Keep remaining Page settings on this feature and coordinate header/footer/footnote/table content with initech-8q6j; do not duplicate P4.

Required oracle: Independent WP setting/code golden plus host page/layout and face tests; emulator Shift-F8 2, change/cancel, save/retrieve and compare pagination. Mutant: open submenu but drop its code/consumer. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-format-main-2.png.


### B31 - IWORD Format: build the Document submenu with a real document setting (P2, feature)

Covers: L053.

Root: os/iword/app/face_a.c:1881-1884 ignores Format choice 3 and defines no Document submenu mode.

Ground truth: ../wordperfect51-decomp/mint/shots_wpMINT21/01_format_menu.png, WP Reference Format Document, ref-f.tex:130-141; no unobserved child defect is asserted.

Pickup: First pickup: mint the menu and one independent Document-setting fixture, implement its input/persistence/layout consumer and cancellation. Account for the rest on this owner; a menu-only shell is not completion.

Required oracle: Host face/codec/layout differential from the real fixture and emulator Shift-F8 3 apply/cancel/save/reload. Mutant: save no setting or ignore it after retrieve. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-format-main-3.png.


### B32 - IWORD Format: build the Other submenu from independently captured behavior (P3, feature)

Covers: L054.

Root: os/iword/app/face_a.c:1881-1884 ignores Format choice 4 and defines no Other submenu mode.

Ground truth: ../wordperfect51-decomp/mint/shots_wpMINT21/01_format_menu.png, WP Reference Format Other, ref-f.tex:130-141. Its children were unreachable in the audit.

Pickup: First pickup: capture the real Other menu and one bounded setting with a genuine consumer; preserve document state on cancel. Retain the remaining submenu scope here rather than inventing child bugs or promising full parity.

Required oracle: Real WP menu/code golden, host input/consumer/round-trip tests, emulator Shift-F8 4 apply/cancel/retrieve. Mutant: accept selection without applying the setting. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-format-main-4.png.


### B33 - IWORD Format Line: implement hyphenation and its zone as one line-break policy (P2, feature)

Covers: L055,L056.

Root: os/iword/app/face_a.c:946-948 displays fixed Hyphenation/Zone values; :1885-1890 handles only Line choices 3/6/7, omitting 1/2.

Ground truth: ../wordperfect51-decomp/mint/shots_wpMINT21/02_format_line.png, WP Reference Hyphenation/Hyphenation Zone, ref-f.tex:135-141; exact hyphen and zone rules need independent fixtures.

Pickup: First pickup: add verified enable/zone input and one measurable hyphenated paragraph through the shared reflow policy, including undo/serialization. Keep broader language/dictionary parity explicit.

Required oracle: Host independently minted line-break/code cases for zone edges, hard spaces and disabled state; emulator change both fields and verify wrapping/save/retrieve. Mutants: ignore zone or hyphenate while off. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-format-line-1.png; docs/audits/2026-10-23-iword/shots/s03-format-line-2.png.


### B34 - IWORD Format Line: add line-height setting and vertical layout support (P2, feature)

Covers: L057.

Root: os/iword/app/face_a.c:950 displays IW_FMT_V_HEIGHT; :1885-1890 omits Line choice 4. Current vertical layout uses fixed line metrics and spacing, not configurable height.

Ground truth: ../wordperfect51-decomp/mint/shots_wpMINT21/02_format_line.png, WP Reference Line Height, ref-f.tex:135-141; line height is distinct from implemented Line Spacing.

Pickup: Implement a captured height contract through field input, document codes and vertical layout/page position. Do not alias it to Line Spacing or silently change existing metric goldens.

Required oracle: Host independent WPU/vertical-position fixture and codec tests; emulator choice 4 apply/cancel and save/reload, observing different line/page positions. Mutant: store height but keep default layout. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-format-line-4.png.


### B35 - IWORD Format Line: implement line numbering with page-aware output (P3, feature)

Covers: L058.

Root: os/iword/app/face_a.c:951 displays a fixed Line Numbering value; :1885-1890 has no Line choice-5 handler or numbering state.

Ground truth: ../wordperfect51-decomp/mint/shots_wpMINT21/02_format_line.png, WP Reference Line Numbering and ref-f.tex:135-141.

Pickup: Capture numbering placement/reset rules and implement the documented supported subset through layout and persisted codes. Coordinate printed numbering with the existing P4 owner initech-8q6j.

Required oracle: Host independent numbering across wraps/hard returns/pages plus codec tests; emulator enable/disable, inspect/retrieve and later P4 print-stream control. Mutant: restart every line or omit numbers. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-format-line-5.png.


### B36 - IWORD Format Line: implement widow and orphan pagination policy (P2, feature)

Covers: L060.

Root: os/iword/app/face_a.c:964 displays IW_FMT_V_WIDOW and :1885-1890 omits Line choice 9; no paragraph-boundary widow/orphan consumer is dispatched.

Ground truth: ../wordperfect51-decomp/mint/shots_wpMINT21/02_format_line.png, WP Reference Widow/Orphan Protection, ref-f.tex:135-141.

Pickup: Add captured protection rules to the shared page-break model, with on/off document setting and preserved soft-page/paragraph semantics. Coordinate B10 soft pages and P4 layout.

Required oracle: Real WP boundary goldens for short/long paragraphs at page ends; host pagination/code tests and emulator change/save/retrieve with page positions. Mutant: always use unprotected pagination. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-format-line-9.png.


### B37 - IWORD: implement Tab Set and Tab Align through one tab-stop model (P2, feature)

Covers: L059,L081.

Root: os/iword/app/face_a.c:963 displays fixed tab settings and :1885-1890 omits Line choice 8; spec/iword/fkeys.h:75 grays Tab Align. os/iword/core/edit.c:63 hardcodes five-column stops.

Ground truth: WP Reference Tab Set/Tab Align, ../wordperfect51-decomp/mint/shots_wpMINT21/02_format_line.png and spec/iword/wp5_format.h:238-246 relative stops; custom/aligned stops require new real-WP fixtures.

Pickup: Implement a shared persisted stop representation, setting editor and Ctrl-F6 alignment entry/consumer, including cursor, reflow and WP payload geometry. Keep long-word boundary adjudication on initech-naoz.

Required oracle: Host independently minted custom-stop/alignment code and position fixtures; emulator configure stops, enter aligned text, edit before it, save/retrieve and compare real WP. Mutants: force default stops or ignore alignment. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s03-format-line-8.png; docs/audits/2026-10-23-iword/shots/s01-key-tab-align.png.


### B38 - IWORD: replace the Help placeholder with reachable context and key help (P2, feature)

Covers: L062.

Root: spec/iword/fkeys.h:30 maps F3 Help to IW_FK_gray; os/iword/app/face_a.c:2086 only displays its gray name, with no help engine/state.

Ground truth: spec/iword/fkeys.json:10 and WP Reference Help; pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks the unbuilt command.

Pickup: First pickup: useful in-app help for actual editing/menu keys and returning to the interrupted state. Keep the real WP topic/navigation contract as the reference and disclose any reduced authored topic set.

Required oracle: Host help routing/topic/state tests and emulator F3 in editing, prompt and block contexts, then return with text/query/selection preserved. Mutant: display command name only or clear the interrupted state. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-key-help.png.


### B39 - IWORD: add merge field codes and a bounded Merge/Sort execution path (P3, feature)

Covers: L063,L069,L083.

Root: spec/iword/fkeys.h:36/:50/:78 maps End Field, Merge Codes and Merge/Sort to gray; os/iword/app/face_a.c:2086 has no merge parser, data-source owner or sort workflow.

Ground truth: spec/iword/fkeys.json:16/:28/:52 and WP Reference Merge Codes/End Field/Merge-Sort; first acquire real form/data/output and sort fixtures. The key table proves entry names, not merge semantics.

Pickup: One shared merge subsystem owner. First pickup: one verified form/data-field stream with End Field and Merge Codes, plus a bounded execution fixture; track sort and wider merge language here until implemented. Never write a success-only stub or guess a macro/merge grammar.

Required oracle: New host real-WP merge/sort differential and malformed-input tests; emulator invoke each route, produce/reopen independent output and preserve both inputs on failure. Mutants: omit field terminator, substitute wrong field or leave sort order unchanged. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-key-end-field.png; docs/audits/2026-10-23-iword/shots/s01-key-merge-codes.png; docs/audits/2026-10-23-iword/shots/s01-key-merge-sort.png.


### B40 - IWORD: implement reverse Indent through the existing paragraph geometry engine (P2, feature)

Covers: L065.

Root: spec/iword/fkeys.h:45 grays Shift-F4; os/iword/app/face_a.c:2066 dispatches forward Indent only. No reverse-indent insertion route exists.

Ground truth: spec/iword/fkeys.json:23, WP Reference Indent/reverse Indent and real-WP bytes/positions to mint before extending spec/iword/wp5_format.h.

Pickup: Add the independently measured reverse-indent action and code/layout rules to the existing indent model, with bounds/cancel/undo. Coordinate right-edge geometry evidence on initech-naoz.

Required oracle: Host real-WP geometry and round-trip fixture, emulator Shift-F4 at normal and boundary positions then edit/undo/save/retrieve. Mutant: invoke forward Indent. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-key-left-indent.png.


### B41 - IWORD: implement the Date/Outline menu with a real dated or numbered result (P2, feature)

Covers: L066.

Root: spec/iword/fkeys.h:46 grays Date/Outline and os/iword/app/face_a.c:2086 has no menu/action state for it.

Ground truth: spec/iword/fkeys.json:24 and WP Reference Date/Outline; capture actual menu, date-code and outline-counter semantics with fixed guest time.

Pickup: First pickup: one verified Date insertion and outline-menu contract, keeping wider numbering schemes on this owner. Consume guest time deterministically in tests and preserve the document on cancel.

Required oracle: Host fixed-clock code/output and outline reset/sequence fixtures; emulator Shift-F5 date/outline choice, cancel, save/retrieve. Mutants: literal key text instead of code or fixed incorrect counter. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-key-date.png.


### B42 - IWORD: implement Mark Text with an independently verified marking consumer (P3, feature)

Covers: L071.

Root: spec/iword/fkeys.h:60 grays Alt-F5 and os/iword/app/face_a.c:2086 has no mark-selection or mark-code action.

Ground truth: spec/iword/fkeys.json:36 and WP Reference Mark Text; acquire actual context menu and marking-code/consumer fixture before choosing the data model.

Pickup: First pickup: one real captured mark operation and its downstream consumer over complete document units. Keep broader reference/index/table-of-contents modes explicit on this feature.

Required oracle: Host real-WP mark-code/consumer differential and cancellation tests; emulator Alt-F5, apply/cancel and save/retrieve with visible or generated marked result. Mutant: keep only the menu or drop marks on serialization. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-key-mark-text.png.


### B43 - IWORD: implement the Face A Style command over persisted style definitions (P3, feature)

Covers: L074.

Root: spec/iword/fkeys.h:63 grays Alt-F8; os/iword/app/face_a.c:2086 has no style-definition/selection state.

Ground truth: spec/iword/fkeys.json:39 and WP Reference Style, with real-WP definition/application fixtures required. P5 Face B on initech-xp4k is a related integration dependency, not proof this WP command exists.

Pickup: First pickup: define/select/apply one verified named style and round-trip it independently, retaining wider style editing modes here. Share a common document model with later Face B work rather than creating view-only styles.

Required oracle: New host style codec/application and real-WP differential; emulator Alt-F8 define/apply/cancel/save/retrieve. Mutants: render style name only or flatten style to direct formatting. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-key-style.png.


### B44 - IWORD: implement Graphics insertion and a bounded native document renderer (P3, feature)

Covers: L075.

Root: spec/iword/fkeys.h:64 grays Alt-F9; os/iword/app/face_a.c:2086 has no graphics menu, resource owner or rendering route.

Ground truth: spec/iword/fkeys.json:40 and WP Reference Graphics; acquire one licensed/local real-WP graphics fixture and its independent layout/code contract before implementation.

Pickup: First pickup: one bounded verified graphic type with import, placement, rendering and persistence; retain unsupported formats/editors here. Coordinate printed output with initech-8q6j.

Required oracle: New host parser/layout/codec differential and malformed fixture tests; emulator Alt-F9 insert/move/cancel/save/retrieve the graphic. Mutants: omit graphic resource or draw a placeholder independent of bytes. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-key-graphics.png.


### B45 - IWORD: implement bounded macro definition and playback as one subsystem (P3, feature)

Covers: L076,L084.

Root: spec/iword/fkeys.h:65/:79 grays Alt-F10 Macro and Ctrl-F10 Macro Define; os/iword/app/face_a.c:2086 has no recorder, storage or playback engine.

Ground truth: spec/iword/fkeys.json:41/:53, WP Reference Macro/Define; INITECH-WORD-plan section 7/P6 explicitly records the full macro language as an era shortfall, not an implemented engine.

Pickup: First pickup: capture and implement a bounded record/save/play/abort command subset with explicit recursion/output limits. Keep full-language exclusions visible and coordinate ledger updates on initech-o9fo; do not silently promise the full language.

Required oracle: Independent real-WP macro fixture and host record/play/input-boundary tests; emulator define then replay text/command macro, cancel and invalid-file controls. Mutants: playback no-op or bypass abort/recursion bound. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-key-macro.png; docs/audits/2026-10-23-iword/shots/s01-key-macro-define.png.


### B46 - IWORD: provide a safe Shell command handoff and return (P3, feature)

Covers: L077.

Root: spec/iword/fkeys.h:70 grays Ctrl-F1; os/iword/app/face_a.c:2086 never requests a shell or saves/resumes an application execution context.

Ground truth: spec/iword/fkeys.json:44, WP Reference Shell and ADR-0013 tenant/loader lifetime contract; foreign 16-bit WP execution is not required.

Pickup: First pickup: native command handoff for VER with reliable return to the same dirty document/cursor/handles. Resolve execution ownership before nesting a shell over the tenant image; retain broader Shell options here.

Required oracle: Host suspend/resume/handle ownership tests and emulator Ctrl-F1 VER -> Word -> save/quit, with child-error/cancel controls. Mutants: clear document on return or skip shell execution. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-key-shell.png.


### B47 - IWORD: implement Screen Rewrite and split-view commands (P3, feature)

Covers: L079.

Root: spec/iword/fkeys.h:72 grays Ctrl-F3; os/iword/app/face_a.c:2086 has no Screen menu or second editing-view state. Existing Reveal split is a separate specialized path.

Ground truth: spec/iword/fkeys.json:46 and WP Reference Screen/Window/Rewrite; exact split allocation/focus requires an independent original screen capture.

Pickup: First pickup: real Rewrite plus one verified split-view operation with independent cursor/viewport and shared document. This is not the P5 Face B renderer and must not allocate a second copy of document bytes.

Required oracle: Host shared-model/two-viewport/focus/repaint tests and emulator Ctrl-F3 Rewrite/split/close with editing visible in both views. Mutants: repaint nothing or let the two views diverge. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-key-screen.png.


### B48 - IWORD: implement bounded Text In/Out conversion through the WP5 core (P2, feature)

Covers: L080.

Root: spec/iword/fkeys.h:74 grays Ctrl-F5; os/iword/app/face_a.c:2086 has no conversion workflow. iw_wp5_text at os/iword/fs/wp5.c:110 is a lossy text extractor, not an import/export command.

Ground truth: spec/iword/fkeys.json:48, WP Reference Text In/Out, and INITECH-WORD-plan P6 .DOC/RTF shortfall limits; acquire an independent ASCII conversion fixture.

Pickup: First pickup: verified plain-text import/export with explicit newline/encoding/code handling, safe file replacement and cancel. Reuse initech-mcep for output safety; retain non-ASCII/foreign formats as explicit remaining scope.

Required oracle: Host independent byte fixtures for hard/soft returns, tabs, extended codes and selected/whole text; emulator Ctrl-F5 import/export then mtools/real-WP readback and failure preservation. Mutant: concatenate paragraphs or silently drop an unsupported character. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-key-text-in-out.png.


### B49 - IWORD: show unavailable Font and Format choices consistently (P3, bug)

Covers: L046,L047.

Root: os/iword/app/face_a.c:786 and :814 render all Font menu text in normal color despite :1767-1773 omitting Base Font/Print Color; paint_format :937-964 paints unsupported groups/settings without live-state styling.

Ground truth: The existing unavailable-choice convention in spec/iword/listfiles.h:115-122 and paint_setup :1030-1044; this is native reduced-feature feedback, not a claim that real WP grays working features.

Pickup: Render and dispatch from explicit availability metadata, using the same gray convention as Setup/List Files. Keep feature delivery on initech-9m65 and the relevant Format feature batches; only those owners may turn a choice live.

Required oracle: Host independent availability expectations and pixel/color assertions for all live/inactive rows; emulator Font and Format screenshots plus still-functional active controls. Mutant: force all items live-colored or disable a working choice. Record clip.

Evidence: docs/audits/2026-10-23-iword/shots/s01-font-menu.png; docs/audits/2026-10-23-iword/shots/s03-format-main.png.


### B50 - IWORD manual: correct Esc instructions for leaving List Files (P3, task)

Covers: L016.

Root: pubs/manuals/initechword-reference/ref-l.tex:87 incorrectly promises Esc exits the list; os/iword/app/face_a.c:1526 exits only on F7/F1. The actual behavior matches real WP LF03_16.

Ground truth: ../wordperfect51-decomp/mint/work/LISTF/mint-results-005.md:126 and the real LF03_16 capture; Look Esc behavior at the same manual line is valid.

Pickup: Correct the list-exit sentence to F7 (and accurately describe the authored F1 convenience); preserve the correct Esc-to-leave-Look instruction. Do not add a nonhistorical Esc action solely to satisfy the wrong manual.

Required oracle: Extend harness/pubs/iword_check.sh or its content probe to check the context-specific instruction; build/render the changed manual and replay list Esc/F7 plus Look Esc. Mutant: restore the false Esc-to-exit-list claim. No system behavior change is required.

Evidence: docs/audits/2026-10-23-iword/shots/s01-list-root.png.

## Completion accounting

- Findings: 91; table has 91 unique IDs, L001-L091.
- Verdicts: 12 D, 60 F, 1 DOC, 16 DUP, 2 LANE, 0 BY-DESIGN.
- New ownership: 73 findings in 50 developer batches/BEADS.tsv data rows.
- Dispatched: L001/L002 -> initech-mcep; no proposed row contains a dispatched or DUP ID.
- Remaining uncertainty: exact real-WP long-word geometry belongs to naoz;
  new rich-feature grammars/screens need the specifically requested mints,
  rather than guessed behavior. Scope slices do not certify full parity.
- Validation: ID partition, TSV field/type/priority checks, ASCII checks and
  evidence-path existence checks are performed on these two outputs. No fix
  acceptance, full replay, live tracker check or green build is claimed.
