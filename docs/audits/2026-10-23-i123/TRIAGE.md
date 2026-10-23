# Initech 123 source triage

Audit: docs/audits/2026-10-23-i123/REPORT.md and NITS.tsv, L001-L259.
Triage date: 2026-10-23. Repository: /home/tobias/Projects/initech-os.
Source citations are repository-relative, inspected at HEAD
0ba7066190df9a117f3b36d641ed832a1ca6e014. The inspected os/i123,
os/flair/event.c, os/milton/kmain.c and spec/i123 sources are unchanged from
the driven audit commit e67b8311092873fe16e6c6d62ad82f894bfb8d7d.
Paths beginning ../ name sibling reference corpora under /home/tobias/Projects.
Short source names resolve as follows: face_a.c = os/i123/app/face_a.c;
sheet.c, fmt.c, range.c, formula.c, eval.c and recalc.c = os/i123/core/;
wk1.c = os/i123/fs/wk1.c; event.c = os/flair/event.c;
tbxgate.c = os/flair/tbxgate.c; kmain.c = os/milton/kmain.c.
Unprefixed shots/ and evidence/ paths in the finding table are relative to
docs/audits/2026-10-23-i123 (not this output directory).

## Finding judged WRONG

L255 (/System): the unavailable command was observed correctly, but requiring
its execution in the current product scope is WRONG. CLAUDE.md:299-300 says
the OS does not run real 16-bit binaries (no v8086, ADR-0001).
docs/plans/INITECH-123-plan.md:186 explicitly directs the /System shell-out
to be an ERA-SHORTFALL stub with a visible not-supported response.
face_a.c:851 and :685-687 do exactly that. The manual/resource describes the
original Lotus shell-out, not an override of this deliberate compatibility
boundary. Verdict BY-DESIGN; no new feature item. This does not prohibit a
separately authorized native-program shell-out design later.

No other new finding was disproved by source/reference inspection. L035 is
a qualified file-byte divergence, not demonstrated corruption or load failure;
it stays P3 until real-Lotus readback establishes a larger user impact.
The safety refusal on L009 is sensible for the current incomplete name engine,
but the missing capability is F, not deliberate canon. Deferred execution in
era_shortfalls.md is a feature backlog, not a blanket BY-DESIGN exemption.
The 116% pie, trailing minus and hourglass canon are unrelated to these findings.
No guest panic was observed; no contextual fail-loud panic has been refiled.

## Method and tracking boundary

Read CLAUDE.md, the 2026-10-05 full-drive TRIAGE format, then this report and
its evidence. Also read AGENTS.md, PRD 6.5, the Initech 123 plan, locked menu/
WK1 data, relevant application/kernel sources and local Lotus references.
This is source triage: no repository edits, builds, tracker writes, commits,
pushes or emulator drives were performed. The acceptance oracles below are
work required of the fixing developer; none is asserted green by this triage.
Only TRIAGE.md and BEADS.tsv were written in the requested output directory.

D = defect in implemented behavior, including a missing input to an implemented
evaluator. F = capability never built at this entry point, including explicit
safe refusals and enabled-looking but unwired controls. BY-DESIGN = deliberate
scope/canon or correct contextual fail-loud behavior. DOC = shipped manual is
wrong. DUP = the same work already has a specific open owner; its underlying
D/F status is stated in the note. LANE = a dispatched lane already owns this
finding; its underlying defect remains described. There are no DOC findings:
the report found no shipped Initech 123 user manual, and the Lotus reference
is external ground truth, not a shipped-manual defect.

OPEN-BEADS.txt and LANES-IN-FLIGHT.txt are the supplied ownership authority,
not a live tracker status certificate. L001 is in lane initech-mcep and L002
is in lane initech-pmw9 (LANES-IN-FLIGHT.txt:12). Neither appears in BEADS.tsv.
initech-68iw is the umbrella epic, not evidence that every missing branch has
a narrow open implementation owner. Its child initech-1z3o explicitly owns
/Print, /Graph, /Data and macros: those leaves are DUP, including Worksheet
Autoexec and Learn. L031 exactly matches initech-68iw.1 line-1 descriptors.
initech-cqej covers decline/Bochs/clip acceptance, not the distinct L003 pending
Save continuation bug. initech-h0ss is shortfall documentation/governance,
not an owner for implementing every listed shortfall.

The report's Still present / Now fixed tables refer to older audit IDs and
do not add new rows: 2026-10-05 L012 remains initech-blx5 (help), L021 remains
initech-68iw.1 (clock), and L080 remains initech-vdgd (exhaustion diagnostics).
This report's L080 is a Clock/International menu leaf. No old ID is silently
reinterpreted or refiled. The shared caution-icon retest is likewise not a
new numbered finding here.

## Shared menu root cause and evidence key

M1: os/i123/core/menu.c:6-13 and :65-71 load a 381-item/82-level tree.
face_a.c:781-784 descends child menus; :786-849 dispatches only the implemented
Quit, Copy/Move, File Retrieve/Save, Range and Worksheet subsets. Unhandled
leaves reach :851 -> unsupported at :685-687, yielding the reported visible
refusal. Worksheet international formats also explicitly refuse at :824.
spec/i123/era_shortfalls.md I123-P3-01..05/08 records the absences; it does not
claim they are working. The 224 leaves share this absent execution mechanism;
they are not 224 independently implemented routines with fresh regressions.

Independently re-read all 762 item/description strings at all 381 named offsets
from ../lotus123-decomp/goldens/lotus-123-2.2-installed-ref/extracted/installed/123.RI:
all matched evidence/menu-verified.json exactly. SHA256:
ac1242e00bcdf6b1b8ec13164f8f1ed6e737d800e81bb5398e2e56e823e970c6.
All 224 unavailable finding screenshot numbers map to that manifest's exact
leaf path and visible refusal. The table gives each leaf, its screenshot pair
and original RI item/description offsets; evidence/menu-plan.json and
evidence/menus-serial.log give the replay and actual events. Existing menu
inventory tests prove words/navigation, not execution of these missing features.

## All 259 new findings

Each row has exactly one verdict and owner. B01-B21 are new candidate batches;
existing bead IDs and lane IDs are not new creates.

| id | verdict | owner | note |
| --- | --- | --- | --- |
| L001 | LANE | initech-mcep | in lane initech-mcep; D underneath: os/i123/app/face_a.c:647 truncates before writing; :656-658 retains failed replacements. Preserve the previous FULL.WK1. No new item. |
| L002 | LANE | initech-pmw9 | in lane initech-pmw9; D underneath: os/i123/app/face_a.c:194,1058-1060 gives a 241-byte buffer to fmt.c:613; stored prefix plus 240 label characters needs another byte before NUL. No new item. |
| L003 | D | B04 | os/milton/kmain.c:5715-5723 asks Quit, services the immediate callback, then equates still resident with cancellation. os/i123/app/face_a.c:1245-1247 has only opened the Save filename prompt; successful write/exit happens later at :660-662. finder_power returns at kmain.c:5738-5739 and retains no pending continuation. |
| L004 | D | B07 | os/i123/app/face_a.c:905-908 classifies $ as VALUE, but os/i123/core/fmt.c:653-668 accepts only an optional sign followed by digits/decimal point; commit at face_a.c:934-947 then attempts formula parsing and refuses $12.34. |
| L005 | F | B10 | Unbuilt cursor editing, explicitly stated at os/i123/app/face_a.c:1053-1054; :1092 drops EDIT Left/Right/Tab and input only appends. |
| L006 | F | B11 | os/i123/app/face_a.c:745 rejects multi-cell TO; range.c:177 accepts only one destination origin; tiling is explicitly deferred. |
| L007 | F | B11 | os/i123/app/face_a.c:869-878 has typed text/single-pointer movement only; no range-anchor state. |
| L008 | D | B06 | os/i123/app/face_a.c:343 uses whole-column i123_cols_fitting (sheet.c:429-439); col_x at :350-351 rejects even the first column when zero full columns fit. paint_border at :445 and paint_pointer at :486-488 therefore omit heading and pointer although Set-Width accepts 1..240 at :719-725. |
| L009 | F | B12 | range.c:48-51 intentionally blocks redefinition with any formula; missing rebinding capability, not silent corruption. |
| L010 | D | B02 | os/i123/app/face_a.c:756-758 and sheet.c:388 recreate erased default cells as allocated BLANKs; 2048-slot capacity never recovers. |
| L011 | D | B08 | os/i123/core/recalc.c:92 initializes have_now=0; eval.c:912-914 correctly requires it. os/i123/app/face_a.c:931,938,974-981 and :699-702 recalculate without supplying now/have_now anywhere in the app. The existing clock service is os/apps/tbx/tbx.h:137-139, tbx_get_clock. |
| L012 | F | B14 | os/i123/app/face_a.c:775 routes Backup to unsupported(); backup creation never executes. |
| L013 | F | B14 | os/i123/app/face_a.c:603 rejects the space before any password suffix/prompt/encoding workflow exists. |
| L014 | D | B19 | os/i123/app/face_a.c:597-610 has no 8.3 length validation and :585-588 collapses the backend refusal into generic I/O. |
| L015 | D | B20 | os/i123/app/face_a.c:303 sends the description to panel_line; :281-283 pads/truncates to SCR_COLS=76 and put_line at :261-262 clips to actual cols(). Menu item text scrolls at :300, but selected descriptions have no scrolling/wrapping path. |
| L016 | F | B12 | formula.c:259-263 resolves names to coordinates; fmt.c:629 calls a decompiler with no name table; name display is deferred. |
| L017 | D | B19 | os/i123/app/face_a.c:585-588 maps full-disk failures to generic I/O; :650-658 also conflates zero/short-write completion. |
| L018 | D | B05 | os/i123/app/face_a.c:864 treats A_GOTO M_RANGE like a slash-menu range prompt instead of returning to READY. |
| L019 | F | B14 | os/i123/core/eval.c:854 explicitly returns an empty filename; the core has no file-context input despite the app retaining g_current. |
| L020 | F | B12 | os/i123/app/face_a.c:869-878 handles text/arrows/accept only in POINT; F3 has no consumer. |
| L021 | F | B16 | os/i123/app/face_a.c:1068-1097 has Home/arrows/page/Tab but no End latch or block scan, and paint_status at :502-508 only paints CALC/CIRC. spec/i123/era_shortfalls.md I123-P3-08 does not turn missing navigation into canon. |
| L022 | F | B21 | os/i123/app/face_a.c:502-508 has no CAPS; event.c:497-518 consumes the lock key, so an app key handler alone would not refresh it. |
| L023 | F | B21 | os/i123/app/face_a.c:502-508 has no NUM and event.c:497-547 exposes no Num Lock latch/state. |
| L024 | F | B09 | os/i123/app/face_a.c:817-849 implements only selected Worksheet formats/settings, erase, widths and recalc; other leaves reach :851/685-687. International date/time explicitly refuses at :824. No undo snapshot/history or Alt-F4 handler exists in key() at :1068-1146. spec/i123/era_shortfalls.md I123-P3-01/05/08 documents these gaps; documentation is not deliberate canon. |
| L025 | F | B10 | No F4 handler in os/i123/app/face_a.c:1068-1146; era_shortfalls I123-P3-08 explicitly defers it. |
| L026 | D | B05 | os/i123/app/face_a.c:578 sets g_notice; :868 only lets Return clear notices with g_error. |
| L027 | F | B15 | os/i123/app/face_a.c:1293-1301 only recalculates geometry/follows the pointer on updateEvt, and the entire face never calls TBX_SETSCROLL/TBX_GETSCROLL. spec/toolbox_gate.h:795-815 intentionally leaves standard bars disabled until a tenant supplies a vertical range; tbxgate.c:1003-1031 already implements that service. |
| L028 | D | B01 | os/i123/app/face_a.c:1074,860 cancels on unmodified vk 0x46; event.c:491 discards extended provenance. Pending text is lost. |
| L029 | F | B14 | os/i123/app/face_a.c:325-333,880-887 implements a typed-name prompt only; no file enumeration or selectable list is connected. |
| L030 | D | B03 | os/i123/core/fmt.c:245-246 rejects scaled magnitude before checking display width; width 30 cannot rescue it. |
| L031 | DUP | initech-68iw.1 | F underneath; exact open line-1 descriptor owner (OPEN-BEADS.txt:90). os/i123/app/face_a.c:407-421 draws address/content/mode without (F0)/(C2). |
| L032 | D | B02 | os/i123/core/sheet.c:250 allocates the new 241-byte label before releasing the old at :259; final live size would fit. |
| L033 | D | B03 | os/i123/core/fmt.c:784-785 has the same ceiling; eval.c:1166-1167 maps conversion failure to ERR. |
| L034 | D | B01 | os/i123/app/face_a.c:1074 expects ASCII 3; event.c:530-547 sends c with Control in modifiers; :1138-1144 appends c. |
| L035 | D | B18 | os/i123/core/range.c:188-191 clones metadata then erases cells; sheet.c:368-377 never shrinks/rebuilds hid_first/hid_last. The transfer also uses direct i123_put_cell at range.c:209-210, so some destination metadata does not pass through grow_extents. os/i123/fs/wk1.c:326-334 serializes the retained arrays unchanged. |
| L036 | F | B09 | Unbuilt: /Worksheet/Global/Format/Date/4 (Long Intn'l); M1; shots/menu-014-{selected,result}.png; RI 0x0794/0x07a4. |
| L037 | F | B09 | Unbuilt: /Worksheet/Global/Format/Date/5 (Short Intn'l); M1; shots/menu-015-{selected,result}.png; RI 0x0820/0x0831. |
| L038 | F | B09 | Unbuilt: /Worksheet/Global/Format/Date/Time/1 (HH:MM:SS AM/PM); M1; shots/menu-017-{selected,result}.png; RI 0x08b3/0x08c5. |
| L039 | F | B09 | Unbuilt: /Worksheet/Global/Format/Date/Time/2 (HH:MM AM/PM); M1; shots/menu-018-{selected,result}.png; RI 0x08c6/0x08d5. |
| L040 | F | B09 | Unbuilt: /Worksheet/Global/Format/Date/Time/3 (Long Intn'l); M1; shots/menu-019-{selected,result}.png; RI 0x08d6/0x08e6. |
| L041 | F | B09 | Unbuilt: /Worksheet/Global/Format/Date/Time/4 (Short Intn'l); M1; shots/menu-020-{selected,result}.png; RI 0x098b/0x099c. |
| L042 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/Interface; M1; shots/menu-040-{selected,result}.png; RI 0x17e4/0x17ee. |
| L043 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/AutoLF/Yes; M1; shots/menu-042-{selected,result}.png; RI 0x1ada/0x1ade. |
| L044 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/AutoLF/No; M1; shots/menu-043-{selected,result}.png; RI 0x1b1f/0x1b22. |
| L045 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/Left; M1; shots/menu-044-{selected,result}.png; RI 0x1831/0x1836. |
| L046 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/Right; M1; shots/menu-045-{selected,result}.png; RI 0x187f/0x1885. |
| L047 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/Top; M1; shots/menu-046-{selected,result}.png; RI 0x18d1/0x18d5. |
| L048 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/Bot; M1; shots/menu-047-{selected,result}.png; RI 0x191a/0x191e. |
| L049 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/Pg-Length; M1; shots/menu-048-{selected,result}.png; RI 0x196c/0x1976. |
| L050 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/Wait/No; M1; shots/menu-050-{selected,result}.png; RI 0x1b1f/0x1b6b. |
| L051 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/Wait/Yes; M1; shots/menu-051-{selected,result}.png; RI 0x1ada/0x1b9d. |
| L052 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/Setup; M1; shots/menu-052-{selected,result}.png; RI 0x19fe/0x1a04. |
| L053 | F | B09 | Unbuilt: /Worksheet/Global/Default/Printer/Name; M1; shots/menu-053-{selected,result}.png; RI 0x1a36/0x1a3b. |
| L054 | F | B09 | Unbuilt: /Worksheet/Global/Default/Directory; M1; shots/menu-055-{selected,result}.png; RI 0x16c3/0x16cd. |
| L055 | F | B09 | Unbuilt: /Worksheet/Global/Default/Status; M1; shots/menu-056-{selected,result}.png; RI 0x1701/0x1708. |
| L056 | F | B09 | Unbuilt: /Worksheet/Global/Default/Update; M1; shots/menu-057-{selected,result}.png; RI 0x1725/0x172c. |
| L057 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Punctuation/A (.,,); M1; shots/menu-061-{selected,result}.png; RI 0x1e22/0x1e2a. |
| L058 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Punctuation/B (,..); M1; shots/menu-062-{selected,result}.png; RI 0x1e77/0x1e7f. |
| L059 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Punctuation/C (.;,); M1; shots/menu-063-{selected,result}.png; RI 0x1ecd/0x1ed5. |
| L060 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Punctuation/D (,;.); M1; shots/menu-064-{selected,result}.png; RI 0x1f22/0x1f2a. |
| L061 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Punctuation/E (., ); M1; shots/menu-065-{selected,result}.png; RI 0x1f78/0x1f80. |
| L062 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Punctuation/F (,. ); M1; shots/menu-066-{selected,result}.png; RI 0x1fcd/0x1fd5. |
| L063 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Punctuation/G (.; ); M1; shots/menu-067-{selected,result}.png; RI 0x2022/0x202a. |
| L064 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Punctuation/H (,; ); M1; shots/menu-068-{selected,result}.png; RI 0x2077/0x207f. |
| L065 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Currency/Prefix; M1; shots/menu-070-{selected,result}.png; RI 0x20cc/0x20d3. |
| L066 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Currency/Suffix; M1; shots/menu-071-{selected,result}.png; RI 0x20f0/0x20f7. |
| L067 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Date/A (MM/DD/YY); M1; shots/menu-073-{selected,result}.png; RI 0x2113/0x2120. |
| L068 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Date/B (DD/MM/YY); M1; shots/menu-074-{selected,result}.png; RI 0x2157/0x2164. |
| L069 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Date/C (DD.MM.YY); M1; shots/menu-075-{selected,result}.png; RI 0x219b/0x21a8. |
| L070 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Date/D (YY-MM-DD); M1; shots/menu-076-{selected,result}.png; RI 0x21df/0x21ec. |
| L071 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Time/A (HH:MM:SS); M1; shots/menu-078-{selected,result}.png; RI 0x2223/0x2230. |
| L072 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Time/B (HH.MM.SS); M1; shots/menu-079-{selected,result}.png; RI 0x2267/0x2274. |
| L073 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Time/C (HH,MM,SS); M1; shots/menu-080-{selected,result}.png; RI 0x22ab/0x22b8. |
| L074 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Time/D (HHhMMmSSs); M1; shots/menu-081-{selected,result}.png; RI 0x22ef/0x22fd. |
| L075 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Negative/Parentheses; M1; shots/menu-083-{selected,result}.png; RI 0x2335/0x2341. |
| L076 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/International/Negative/Sign; M1; shots/menu-084-{selected,result}.png; RI 0x236d/0x2372. |
| L077 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Help/Instant; M1; shots/menu-087-{selected,result}.png; RI 0x23a2/0x23aa. |
| L078 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Help/Removable; M1; shots/menu-088-{selected,result}.png; RI 0x23d2/0x23dc. |
| L079 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Clock/Standard; M1; shots/menu-090-{selected,result}.png; RI 0x240d/0x2416. |
| L080 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Clock/International; M1; shots/menu-091-{selected,result}.png; RI 0x1bc8/0x244f. |
| L081 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Clock/None; M1; shots/menu-092-{selected,result}.png; RI 0x248e/0x2493. |
| L082 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Clock/Clock; M1; shots/menu-093-{selected,result}.png; RI 0x1c27/0x24b0. |
| L083 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Clock/Filename; M1; shots/menu-094-{selected,result}.png; RI 0x24d4/0x24dd. |
| L084 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Undo/Enable; M1; shots/menu-096-{selected,result}.png; RI 0x1642/0x251a. |
| L085 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Undo/Disable; M1; shots/menu-097-{selected,result}.png; RI 0x1666/0x2527. |
| L086 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Beep/Enable; M1; shots/menu-099-{selected,result}.png; RI 0x1642/0x2535. |
| L087 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Beep/Disable; M1; shots/menu-100-{selected,result}.png; RI 0x1666/0x2563. |
| L088 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Add-In/Set; M1; shots/menu-102-{selected,result}.png; RI 0x2595/0x2599. |
| L089 | F | B09 | Unbuilt: /Worksheet/Global/Default/Other/Add-In/Cancel; M1; shots/menu-103-{selected,result}.png; RI 0x25d7/0x25de. |
| L090 | DUP | initech-1z3o | /Worksheet/Global/Default/Autoexec/Enable; M1; shots/menu-105-{selected,result}.png; RI 0x1642/0x273d. F underneath; Autoexec enable/disable and Learn recorder are macro execution/settings in the open P4 macros child (OPEN-BEADS.txt:88); M1 fallback. |
| L091 | DUP | initech-1z3o | /Worksheet/Global/Default/Autoexec/Disable; M1; shots/menu-106-{selected,result}.png; RI 0x1666/0x276a. F underneath; Autoexec enable/disable and Learn recorder are macro execution/settings in the open P4 macros child (OPEN-BEADS.txt:88); M1 fallback. |
| L092 | F | B09 | Unbuilt: /Worksheet/Global/Zero; M1; shots/menu-108-{selected,result}.png; RI 0x14ca/0x14cf. |
| L093 | F | B09 | Unbuilt: /Worksheet/Insert/Column; M1; shots/menu-110-{selected,result}.png; RI 0x0c46/0x0d9d. |
| L094 | F | B09 | Unbuilt: /Worksheet/Insert/Row; M1; shots/menu-111-{selected,result}.png; RI 0x0dfa/0x0dfe. |
| L095 | F | B09 | Unbuilt: /Worksheet/Delete/Column; M1; shots/menu-113-{selected,result}.png; RI 0x0c46/0x0e4c. |
| L096 | F | B09 | Unbuilt: /Worksheet/Delete/Row; M1; shots/menu-114-{selected,result}.png; RI 0x0dfa/0x0e8a. |
| L097 | F | B09 | Unbuilt: /Worksheet/Column/Hide; M1; shots/menu-118-{selected,result}.png; RI 0x0f44/0x0f49. |
| L098 | F | B09 | Unbuilt: /Worksheet/Column/Display; M1; shots/menu-119-{selected,result}.png; RI 0x0f7a/0x0f82. |
| L099 | F | B09 | Unbuilt: /Worksheet/Column/Column-Range/Set-Width; M1; shots/menu-121-{selected,result}.png; RI 0x0ec2/0x0ffe. |
| L100 | F | B09 | Unbuilt: /Worksheet/Column/Column-Range/Reset-Width; M1; shots/menu-122-{selected,result}.png; RI 0x0f0b/0x104c. |
| L101 | F | B09 | Unbuilt: /Worksheet/Titles/Both; M1; shots/menu-127-{selected,result}.png; RI 0x10d7/0x10dc. |
| L102 | F | B09 | Unbuilt: /Worksheet/Titles/Horizontal; M1; shots/menu-128-{selected,result}.png; RI 0x1122/0x112d. |
| L103 | F | B09 | Unbuilt: /Worksheet/Titles/Vertical; M1; shots/menu-129-{selected,result}.png; RI 0x1154/0x115d. |
| L104 | F | B09 | Unbuilt: /Worksheet/Titles/Clear; M1; shots/menu-130-{selected,result}.png; RI 0x1190/0x1196. |
| L105 | F | B09 | Unbuilt: /Worksheet/Window/Horizontal; M1; shots/menu-132-{selected,result}.png; RI 0x1122/0x11ba. |
| L106 | F | B09 | Unbuilt: /Worksheet/Window/Vertical; M1; shots/menu-133-{selected,result}.png; RI 0x1154/0x11eb. |
| L107 | F | B09 | Unbuilt: /Worksheet/Window/Sync; M1; shots/menu-134-{selected,result}.png; RI 0x121d/0x1222. |
| L108 | F | B09 | Unbuilt: /Worksheet/Window/Unsync; M1; shots/menu-135-{selected,result}.png; RI 0x1243/0x124a. |
| L109 | F | B09 | Unbuilt: /Worksheet/Window/Clear; M1; shots/menu-136-{selected,result}.png; RI 0x1190/0x1267. |
| L110 | F | B09 | Unbuilt: /Worksheet/Status; M1; shots/menu-137-{selected,result}.png; RI 0x0d0a/0x0d11. |
| L111 | F | B09 | Unbuilt: /Worksheet/Page; M1; shots/menu-138-{selected,result}.png; RI 0x0d2c/0x0d31. |
| L112 | DUP | initech-1z3o | /Worksheet/Learn/Range; M1; shots/menu-140-{selected,result}.png; RI 0x1285/0x128b. F underneath; Autoexec enable/disable and Learn recorder are macro execution/settings in the open P4 macros child (OPEN-BEADS.txt:88); M1 fallback. |
| L113 | DUP | initech-1z3o | /Worksheet/Learn/Cancel; M1; shots/menu-141-{selected,result}.png; RI 0x12be/0x12c5. F underneath; Autoexec enable/disable and Learn recorder are macro execution/settings in the open P4 macros child (OPEN-BEADS.txt:88); M1 fallback. |
| L114 | DUP | initech-1z3o | /Worksheet/Learn/Erase; M1; shots/menu-142-{selected,result}.png; RI 0x0c81/0x12f0. F underneath; Autoexec enable/disable and Learn recorder are macro execution/settings in the open P4 macros child (OPEN-BEADS.txt:88); M1 fallback. |
| L115 | F | B13 | Unbuilt: /Range/Format/Date/4 (Long Intn'l); M1; shots/menu-156-{selected,result}.png; RI 0x0794/0x07a4. |
| L116 | F | B13 | Unbuilt: /Range/Format/Date/5 (Short Intn'l); M1; shots/menu-157-{selected,result}.png; RI 0x0820/0x0831. |
| L117 | F | B13 | Unbuilt: /Range/Format/Date/Time/1 (HH:MM:SS AM/PM); M1; shots/menu-159-{selected,result}.png; RI 0x08b3/0x08c5. |
| L118 | F | B13 | Unbuilt: /Range/Format/Date/Time/2 (HH:MM AM/PM); M1; shots/menu-160-{selected,result}.png; RI 0x08c6/0x08d5. |
| L119 | F | B13 | Unbuilt: /Range/Format/Date/Time/3 (Long Intn'l); M1; shots/menu-161-{selected,result}.png; RI 0x08d6/0x08e6. |
| L120 | F | B13 | Unbuilt: /Range/Format/Date/Time/4 (Short Intn'l); M1; shots/menu-162-{selected,result}.png; RI 0x098b/0x099c. |
| L121 | F | B13 | Unbuilt: /Range/Justify; M1; shots/menu-181-{selected,result}.png; RI 0x28bb/0x28c3. |
| L122 | F | B13 | Unbuilt: /Range/Input; M1; shots/menu-184-{selected,result}.png; RI 0x29b3/0x29b9. |
| L123 | F | B13 | Unbuilt: /Range/Search/Formulas/Find; M1; shots/menu-189-{selected,result}.png; RI 0x2d53/0x2d58. |
| L124 | F | B13 | Unbuilt: /Range/Search/Formulas/Replace; M1; shots/menu-190-{selected,result}.png; RI 0x2d84/0x2d8c. |
| L125 | F | B13 | Unbuilt: /Range/Search/Label; M1; shots/menu-191-{selected,result}.png; RI 0x2807/0x2d04. |
| L126 | F | B13 | Unbuilt: /Range/Search/Both; M1; shots/menu-192-{selected,result}.png; RI 0x2d25/0x2d2a. |
| L127 | F | B14 | Unbuilt: /File/Combine/Copy/Entire-File; M1; shots/menu-200-{selected,result}.png; RI 0x3342/0x334e. |
| L128 | F | B14 | Unbuilt: /File/Combine/Copy/Named/Specified-Range; M1; shots/menu-201-{selected,result}.png; RI 0x3375/0x338b. |
| L129 | F | B14 | Unbuilt: /File/Combine/Add/Entire-File; M1; shots/menu-203-{selected,result}.png; RI 0x3342/0x334e. |
| L130 | F | B14 | Unbuilt: /File/Combine/Add/Named/Specified-Range; M1; shots/menu-204-{selected,result}.png; RI 0x3375/0x338b. |
| L131 | F | B14 | Unbuilt: /File/Combine/Subtract/Entire-File; M1; shots/menu-206-{selected,result}.png; RI 0x3342/0x334e. |
| L132 | F | B14 | Unbuilt: /File/Combine/Subtract/Named/Specified-Range; M1; shots/menu-207-{selected,result}.png; RI 0x3375/0x338b. |
| L133 | F | B14 | Unbuilt: /File/Xtract/Formulas; M1; shots/menu-209-{selected,result}.png; RI 0x33dc/0x33e5. |
| L134 | F | B14 | Unbuilt: /File/Xtract/Values; M1; shots/menu-210-{selected,result}.png; RI 0x3402/0x3409. |
| L135 | F | B14 | Unbuilt: /File/Erase/Worksheet; M1; shots/menu-212-{selected,result}.png; RI 0x3428/0x3432. |
| L136 | F | B14 | Unbuilt: /File/Erase/Print; M1; shots/menu-213-{selected,result}.png; RI 0x3449/0x344f. |
| L137 | F | B14 | Unbuilt: /File/Erase/Graph; M1; shots/menu-214-{selected,result}.png; RI 0x3462/0x3468. |
| L138 | F | B14 | Unbuilt: /File/Erase/Other; M1; shots/menu-215-{selected,result}.png; RI 0x347b/0x3481. |
| L139 | F | B14 | Unbuilt: /File/List/Worksheet; M1; shots/menu-217-{selected,result}.png; RI 0x3428/0x34c7. |
| L140 | F | B14 | Unbuilt: /File/List/Print; M1; shots/menu-218-{selected,result}.png; RI 0x3449/0x34dc. |
| L141 | F | B14 | Unbuilt: /File/List/Graph; M1; shots/menu-219-{selected,result}.png; RI 0x3462/0x34ed. |
| L142 | F | B14 | Unbuilt: /File/List/Other; M1; shots/menu-220-{selected,result}.png; RI 0x347b/0x34fe. |
| L143 | F | B14 | Unbuilt: /File/List/Linked; M1; shots/menu-221-{selected,result}.png; RI 0x350d/0x3514. |
| L144 | F | B14 | Unbuilt: /File/Import/Text; M1; shots/menu-223-{selected,result}.png; RI 0x3537/0x353c. |
| L145 | F | B14 | Unbuilt: /File/Import/Numbers; M1; shots/menu-224-{selected,result}.png; RI 0x3567/0x356f. |
| L146 | F | B14 | Unbuilt: /File/Directory; M1; shots/menu-225-{selected,result}.png; RI 0x31b1/0x31bb. |
| L147 | F | B14 | Unbuilt: /File/Admin/Reservation/Get; M1; shots/menu-228-{selected,result}.png; RI 0x3638/0x363c. |
| L148 | F | B14 | Unbuilt: /File/Admin/Reservation/Release; M1; shots/menu-229-{selected,result}.png; RI 0x3665/0x366d. |
| L149 | F | B14 | Unbuilt: /File/Admin/Table; M1; shots/menu-230-{selected,result}.png; RI 0x35de/0x35e4. |
| L150 | F | B14 | Unbuilt: /File/Admin/Link-Refresh; M1; shots/menu-231-{selected,result}.png; RI 0x3617/0x3624. |
| L151 | DUP | initech-1z3o | /Print/Printer/Range; M1; shots/menu-234-{selected,result}.png; RI 0x3754/0x375a. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L152 | DUP | initech-1z3o | /Print/Printer/Line; M1; shots/menu-235-{selected,result}.png; RI 0x3787/0x378c. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L153 | DUP | initech-1z3o | /Print/Printer/Page; M1; shots/menu-236-{selected,result}.png; RI 0x37a3/0x37a8. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L154 | DUP | initech-1z3o | /Print/Printer/Options/Header; M1; shots/menu-238-{selected,result}.png; RI 0x389e/0x38a5. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L155 | DUP | initech-1z3o | /Print/Printer/Options/Footer; M1; shots/menu-239-{selected,result}.png; RI 0x38c4/0x38cb. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L156 | DUP | initech-1z3o | /Print/Printer/Options/Margins/Left; M1; shots/menu-241-{selected,result}.png; RI 0x3a21/0x3a26. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L157 | DUP | initech-1z3o | /Print/Printer/Options/Margins/Right; M1; shots/menu-242-{selected,result}.png; RI 0x3a67/0x3a6d. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L158 | DUP | initech-1z3o | /Print/Printer/Options/Margins/Top; M1; shots/menu-243-{selected,result}.png; RI 0x3ab1/0x3ab5. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L159 | DUP | initech-1z3o | /Print/Printer/Options/Margins/Bottom; M1; shots/menu-244-{selected,result}.png; RI 0x3af2/0x3af9. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L160 | DUP | initech-1z3o | /Print/Printer/Options/Margins/None; M1; shots/menu-245-{selected,result}.png; RI 0x3b3f/0x3b44. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L161 | DUP | initech-1z3o | /Print/Printer/Options/Borders/Columns; M1; shots/menu-247-{selected,result}.png; RI 0x3b5e/0x3b66. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L162 | DUP | initech-1z3o | /Print/Printer/Options/Borders/Rows; M1; shots/menu-248-{selected,result}.png; RI 0x3bbc/0x3bc1. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L163 | DUP | initech-1z3o | /Print/Printer/Options/Setup; M1; shots/menu-249-{selected,result}.png; RI 0x393a/0x3940. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L164 | DUP | initech-1z3o | /Print/Printer/Options/Pg-Length; M1; shots/menu-250-{selected,result}.png; RI 0x3970/0x397a. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L165 | DUP | initech-1z3o | /Print/Printer/Options/Other/As-Displayed; M1; shots/menu-252-{selected,result}.png; RI 0x3c08/0x3c15. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L166 | DUP | initech-1z3o | /Print/Printer/Options/Other/Cell-Formulas; M1; shots/menu-253-{selected,result}.png; RI 0x3c2e/0x3c3c. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L167 | DUP | initech-1z3o | /Print/Printer/Options/Other/Formatted; M1; shots/menu-254-{selected,result}.png; RI 0x3c57/0x3c61. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L168 | DUP | initech-1z3o | /Print/Printer/Options/Other/Unformatted; M1; shots/menu-255-{selected,result}.png; RI 0x3c89/0x3c95. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L169 | DUP | initech-1z3o | /Print/Printer/Clear/All; M1; shots/menu-258-{selected,result}.png; RI 0x3cc4/0x3cc8. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L170 | DUP | initech-1z3o | /Print/Printer/Clear/Range; M1; shots/menu-259-{selected,result}.png; RI 0x3754/0x3cee. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L171 | DUP | initech-1z3o | /Print/Printer/Clear/Borders; M1; shots/menu-260-{selected,result}.png; RI 0x3911/0x3d08. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L172 | DUP | initech-1z3o | /Print/Printer/Clear/Format; M1; shots/menu-261-{selected,result}.png; RI 0x3d2b/0x3d32. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L173 | DUP | initech-1z3o | /Print/Printer/Align; M1; shots/menu-262-{selected,result}.png; RI 0x3834/0x383a. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L174 | DUP | initech-1z3o | /Print/Printer/Go; M1; shots/menu-263-{selected,result}.png; RI 0x3867/0x386a. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L175 | DUP | initech-1z3o | /Print/File/Range; M1; shots/menu-266-{selected,result}.png; RI 0x3754/0x375a. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L176 | DUP | initech-1z3o | /Print/File/Line; M1; shots/menu-267-{selected,result}.png; RI 0x3787/0x378c. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L177 | DUP | initech-1z3o | /Print/File/Page; M1; shots/menu-268-{selected,result}.png; RI 0x37a3/0x37a8. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L178 | DUP | initech-1z3o | /Print/File/Options/Header; M1; shots/menu-270-{selected,result}.png; RI 0x389e/0x38a5. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L179 | DUP | initech-1z3o | /Print/File/Options/Footer; M1; shots/menu-271-{selected,result}.png; RI 0x38c4/0x38cb. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L180 | DUP | initech-1z3o | /Print/File/Options/Margins/Left; M1; shots/menu-273-{selected,result}.png; RI 0x3a21/0x3a26. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L181 | DUP | initech-1z3o | /Print/File/Options/Margins/Right; M1; shots/menu-274-{selected,result}.png; RI 0x3a67/0x3a6d. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L182 | DUP | initech-1z3o | /Print/File/Options/Margins/Top; M1; shots/menu-275-{selected,result}.png; RI 0x3ab1/0x3ab5. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L183 | DUP | initech-1z3o | /Print/File/Options/Margins/Bottom; M1; shots/menu-276-{selected,result}.png; RI 0x3af2/0x3af9. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L184 | DUP | initech-1z3o | /Print/File/Options/Margins/None; M1; shots/menu-277-{selected,result}.png; RI 0x3b3f/0x3b44. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L185 | DUP | initech-1z3o | /Print/File/Options/Borders/Columns; M1; shots/menu-279-{selected,result}.png; RI 0x3b5e/0x3b66. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L186 | DUP | initech-1z3o | /Print/File/Options/Borders/Rows; M1; shots/menu-280-{selected,result}.png; RI 0x3bbc/0x3bc1. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L187 | DUP | initech-1z3o | /Print/File/Options/Setup; M1; shots/menu-281-{selected,result}.png; RI 0x393a/0x3940. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L188 | DUP | initech-1z3o | /Print/File/Options/Pg-Length; M1; shots/menu-282-{selected,result}.png; RI 0x3970/0x397a. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L189 | DUP | initech-1z3o | /Print/File/Options/Other/As-Displayed; M1; shots/menu-284-{selected,result}.png; RI 0x3c08/0x3c15. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L190 | DUP | initech-1z3o | /Print/File/Options/Other/Cell-Formulas; M1; shots/menu-285-{selected,result}.png; RI 0x3c2e/0x3c3c. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L191 | DUP | initech-1z3o | /Print/File/Options/Other/Formatted; M1; shots/menu-286-{selected,result}.png; RI 0x3c57/0x3c61. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L192 | DUP | initech-1z3o | /Print/File/Options/Other/Unformatted; M1; shots/menu-287-{selected,result}.png; RI 0x3c89/0x3c95. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L193 | DUP | initech-1z3o | /Print/File/Clear/All; M1; shots/menu-290-{selected,result}.png; RI 0x3cc4/0x3cc8. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L194 | DUP | initech-1z3o | /Print/File/Clear/Range; M1; shots/menu-291-{selected,result}.png; RI 0x3754/0x3cee. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L195 | DUP | initech-1z3o | /Print/File/Clear/Borders; M1; shots/menu-292-{selected,result}.png; RI 0x3911/0x3d08. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L196 | DUP | initech-1z3o | /Print/File/Clear/Format; M1; shots/menu-293-{selected,result}.png; RI 0x3d2b/0x3d32. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L197 | DUP | initech-1z3o | /Print/File/Align; M1; shots/menu-294-{selected,result}.png; RI 0x3834/0x383a. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L198 | DUP | initech-1z3o | /Print/File/Go; M1; shots/menu-295-{selected,result}.png; RI 0x3867/0x386a. F underneath; /Print execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L199 | DUP | initech-1z3o | /Graph/Type/Line; M1; shots/menu-299-{selected,result}.png; RI 0x403c/0x4041. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L200 | DUP | initech-1z3o | /Graph/Type/Bar; M1; shots/menu-300-{selected,result}.png; RI 0x404c/0x4050. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L201 | DUP | initech-1z3o | /Graph/Type/XY; M1; shots/menu-301-{selected,result}.png; RI 0x405a/0x405d. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L202 | DUP | initech-1z3o | /Graph/Type/Stack-Bar; M1; shots/menu-302-{selected,result}.png; RI 0x4066/0x4070. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L203 | DUP | initech-1z3o | /Graph/Type/Pie; M1; shots/menu-303-{selected,result}.png; RI 0x4082/0x4086. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L204 | DUP | initech-1z3o | /Graph/X; M1; shots/menu-304-{selected,result}.png; RI 0x3d8f/0x3d91. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L205 | DUP | initech-1z3o | /Graph/A; M1; shots/menu-305-{selected,result}.png; RI 0x3db7/0x3db9. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L206 | DUP | initech-1z3o | /Graph/B; M1; shots/menu-306-{selected,result}.png; RI 0x3de7/0x3de9. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L207 | DUP | initech-1z3o | /Graph/C; M1; shots/menu-307-{selected,result}.png; RI 0x3e19/0x3e1b. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L208 | DUP | initech-1z3o | /Graph/D; M1; shots/menu-308-{selected,result}.png; RI 0x3e49/0x3e4b. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L209 | DUP | initech-1z3o | /Graph/E; M1; shots/menu-309-{selected,result}.png; RI 0x3e7b/0x3e7d. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L210 | DUP | initech-1z3o | /Graph/F; M1; shots/menu-310-{selected,result}.png; RI 0x3eab/0x3ead. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L211 | DUP | initech-1z3o | /Graph/Reset; M1; shots/menu-311-{selected,result}.png; RI 0x3edb/0x3ee1. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L212 | DUP | initech-1z3o | /Graph/View; M1; shots/menu-312-{selected,result}.png; RI 0x3f13/0x3f18. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L213 | DUP | initech-1z3o | /Graph/Save; M1; shots/menu-313-{selected,result}.png; RI 0x3f2f/0x3f34. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L214 | DUP | initech-1z3o | /Graph/Options; M1; shots/menu-314-{selected,result}.png; RI 0x3f7a/0x3f82. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L215 | DUP | initech-1z3o | /Graph/Name/Use; M1; shots/menu-316-{selected,result}.png; RI 0x4d17/0x4d1b. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L216 | DUP | initech-1z3o | /Graph/Name/Create; M1; shots/menu-317-{selected,result}.png; RI 0x4d5c/0x4d63. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L217 | DUP | initech-1z3o | /Graph/Name/Delete; M1; shots/menu-318-{selected,result}.png; RI 0x4d8d/0x4d94. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L218 | DUP | initech-1z3o | /Graph/Name/Reset; M1; shots/menu-319-{selected,result}.png; RI 0x3edb/0x4dc9. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L219 | DUP | initech-1z3o | /Graph/Name/Table; M1; shots/menu-320-{selected,result}.png; RI 0x4de1/0x4de7. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L220 | DUP | initech-1z3o | /Graph/Group/Columnwise; M1; shots/menu-322-{selected,result}.png; RI 0x4e1e/0x4e29. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L221 | DUP | initech-1z3o | /Graph/Group/Rowwise; M1; shots/menu-323-{selected,result}.png; RI 0x4e44/0x4e4c. F underneath; /Graph execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L222 | DUP | initech-1z3o | /Data/Fill; M1; shots/menu-326-{selected,result}.png; RI 0x4e64/0x4e69. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L223 | DUP | initech-1z3o | /Data/Table/1; M1; shots/menu-328-{selected,result}.png; RI 0x503c/0x503e. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L224 | DUP | initech-1z3o | /Data/Table/2; M1; shots/menu-329-{selected,result}.png; RI 0x5096/0x5098. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L225 | DUP | initech-1z3o | /Data/Table/Reset; M1; shots/menu-330-{selected,result}.png; RI 0x50d4/0x50da. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L226 | DUP | initech-1z3o | /Data/Sort/Data-Range; M1; shots/menu-332-{selected,result}.png; RI 0x5111/0x511c. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L227 | DUP | initech-1z3o | /Data/Sort/Primary-Key; M1; shots/menu-333-{selected,result}.png; RI 0x514b/0x5157. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L228 | DUP | initech-1z3o | /Data/Sort/Secondary-Key; M1; shots/menu-334-{selected,result}.png; RI 0x51a6/0x51b4. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L229 | DUP | initech-1z3o | /Data/Sort/Reset; M1; shots/menu-335-{selected,result}.png; RI 0x50d4/0x51f9. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L230 | DUP | initech-1z3o | /Data/Sort/Go; M1; shots/menu-336-{selected,result}.png; RI 0x3867/0x521b. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L231 | DUP | initech-1z3o | /Data/Query/Input; M1; shots/menu-339-{selected,result}.png; RI 0x5258/0x525e. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L232 | DUP | initech-1z3o | /Data/Query/Criteria; M1; shots/menu-340-{selected,result}.png; RI 0x52a0/0x52a9. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L233 | DUP | initech-1z3o | /Data/Query/Output; M1; shots/menu-341-{selected,result}.png; RI 0x52e9/0x52f0. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L234 | DUP | initech-1z3o | /Data/Query/Find; M1; shots/menu-342-{selected,result}.png; RI 0x5328/0x532d. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L235 | DUP | initech-1z3o | /Data/Query/Extract; M1; shots/menu-343-{selected,result}.png; RI 0x5359/0x5361. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L236 | DUP | initech-1z3o | /Data/Query/Unique; M1; shots/menu-344-{selected,result}.png; RI 0x5396/0x539d. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L237 | DUP | initech-1z3o | /Data/Query/Delete; M1; shots/menu-345-{selected,result}.png; RI 0x53e6/0x53ed. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L238 | DUP | initech-1z3o | /Data/Query/Reset; M1; shots/menu-346-{selected,result}.png; RI 0x50d4/0x5414. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L239 | DUP | initech-1z3o | /Data/Distribution; M1; shots/menu-348-{selected,result}.png; RI 0x4f26/0x4f33. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L240 | DUP | initech-1z3o | /Data/Matrix/Invert; M1; shots/menu-350-{selected,result}.png; RI 0x54a9/0x54b0. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L241 | DUP | initech-1z3o | /Data/Matrix/Multiply; M1; shots/menu-351-{selected,result}.png; RI 0x54e7/0x54f0. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L242 | DUP | initech-1z3o | /Data/Regression/X-Range; M1; shots/menu-353-{selected,result}.png; RI 0x5551/0x5559. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L243 | DUP | initech-1z3o | /Data/Regression/Y-Range; M1; shots/menu-354-{selected,result}.png; RI 0x55ab/0x55b3. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L244 | DUP | initech-1z3o | /Data/Regression/Output-Range; M1; shots/menu-355-{selected,result}.png; RI 0x55ff/0x560c. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L245 | DUP | initech-1z3o | /Data/Regression/Intercept/Compute; M1; shots/menu-357-{selected,result}.png; RI 0x56b6/0x56be. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L246 | DUP | initech-1z3o | /Data/Regression/Intercept/Zero; M1; shots/menu-358-{selected,result}.png; RI 0x56eb/0x56f0. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L247 | DUP | initech-1z3o | /Data/Regression/Reset; M1; shots/menu-359-{selected,result}.png; RI 0x50d4/0x563d. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L248 | DUP | initech-1z3o | /Data/Regression/Go; M1; shots/menu-360-{selected,result}.png; RI 0x3867/0x5686. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L249 | DUP | initech-1z3o | /Data/Parse/Format-Line/Create; M1; shots/menu-364-{selected,result}.png; RI 0x583a/0x5841. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L250 | DUP | initech-1z3o | /Data/Parse/Format-Line/Edit; M1; shots/menu-365-{selected,result}.png; RI 0x586a/0x586f. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L251 | DUP | initech-1z3o | /Data/Parse/Input-Column; M1; shots/menu-366-{selected,result}.png; RI 0x574e/0x575b. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L252 | DUP | initech-1z3o | /Data/Parse/Output-Range; M1; shots/menu-367-{selected,result}.png; RI 0x55ff/0x57a1. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L253 | DUP | initech-1z3o | /Data/Parse/Reset; M1; shots/menu-368-{selected,result}.png; RI 0x50d4/0x57d2. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L254 | DUP | initech-1z3o | /Data/Parse/Go; M1; shots/menu-369-{selected,result}.png; RI 0x3867/0x57f6. F underneath; /Data execution is explicitly in the open P4 child (OPEN-BEADS.txt:88); M1 fallback, not a new regression. |
| L255 | BY-DESIGN | ADR-0001 / no-v8086 | WRONG as required current-scope feature: CLAUDE.md:299-300 excludes real 16-bit binaries; docs/plans/INITECH-123-plan.md:186 explicitly permits /System to refuse. os/i123/app/face_a.c:851,685-687 supplies the visible refusal. Evidence: shots/menu-371-result.png. |
| L256 | F | B17 | Unbuilt: /Add-In/Attach; M1; shots/menu-373-{selected,result}.png; RI 0x03a0/0x03a7. |
| L257 | F | B17 | Unbuilt: /Add-In/Detach; M1; shots/menu-374-{selected,result}.png; RI 0x03e3/0x03ea. |
| L258 | F | B17 | Unbuilt: /Add-In/Invoke; M1; shots/menu-375-{selected,result}.png; RI 0x0431/0x0438. |
| L259 | F | B17 | Unbuilt: /Add-In/Clear; M1; shots/menu-376-{selected,result}.png; RI 0x0475/0x047b. |

## Existing owners: grouped pickups, no new TSV rows

The two P1 data-loss fixes remain first in user-harm order: L001 on initech-mcep must preserve the
last successful FULL.WK1 across no-space/short-write/close/rename failure, retaining dirty in-memory
work. Its independent byte proof is evidence/full-replace-byte-proof.json and
evidence/extracted/full2/FULL.WK1. L002 on initech-pmw9 must preserve prefix plus all 240 accepted
characters through F2/Return, with evidence/long-edit-byte-proof.json and LONG0.WK1/LONGEDIT.WK1.
Require failure injection, independent stopped-image parsing, QEMU/Bochs replay and a mutant
restoring each destructive/truncating path; these are acceptance requirements, not new work items.

### /Print: initech-1z3o (48 leaves)

F underneath; DUP by OPEN-BEADS.txt:88. Root cause M1. Ground truth: the independently checked real
123.RI offsets above, the Lotus Reference chapter for /Print, and real command-minted output
fixtures. Extend the existing P4 owner with actual host command/output differentials plus locked
QEMU/Bochs execution and record-flair clips. Mutants making the command a visible or silent no-op
must fail. A navigable menu or a removed refusal is not acceptance.

Leaves: L151 /Print/Printer/Range; L152 /Print/Printer/Line; L153 /Print/Printer/Page; L154
/Print/Printer/Options/Header; L155 /Print/Printer/Options/Footer; L156
/Print/Printer/Options/Margins/Left; L157 /Print/Printer/Options/Margins/Right; L158
/Print/Printer/Options/Margins/Top; L159 /Print/Printer/Options/Margins/Bottom; L160
/Print/Printer/Options/Margins/None; L161 /Print/Printer/Options/Borders/Columns; L162
/Print/Printer/Options/Borders/Rows; L163 /Print/Printer/Options/Setup; L164
/Print/Printer/Options/Pg-Length; L165 /Print/Printer/Options/Other/As-Displayed; L166
/Print/Printer/Options/Other/Cell-Formulas; L167 /Print/Printer/Options/Other/Formatted; L168
/Print/Printer/Options/Other/Unformatted; L169 /Print/Printer/Clear/All; L170
/Print/Printer/Clear/Range; L171 /Print/Printer/Clear/Borders; L172 /Print/Printer/Clear/Format;
L173 /Print/Printer/Align; L174 /Print/Printer/Go; L175 /Print/File/Range; L176 /Print/File/Line;
L177 /Print/File/Page; L178 /Print/File/Options/Header; L179 /Print/File/Options/Footer; L180
/Print/File/Options/Margins/Left; L181 /Print/File/Options/Margins/Right; L182
/Print/File/Options/Margins/Top; L183 /Print/File/Options/Margins/Bottom; L184
/Print/File/Options/Margins/None; L185 /Print/File/Options/Borders/Columns; L186
/Print/File/Options/Borders/Rows; L187 /Print/File/Options/Setup; L188
/Print/File/Options/Pg-Length; L189 /Print/File/Options/Other/As-Displayed; L190
/Print/File/Options/Other/Cell-Formulas; L191 /Print/File/Options/Other/Formatted; L192
/Print/File/Options/Other/Unformatted; L193 /Print/File/Clear/All; L194 /Print/File/Clear/Range;
L195 /Print/File/Clear/Borders; L196 /Print/File/Clear/Format; L197 /Print/File/Align; L198
/Print/File/Go.

### /Graph: initech-1z3o (23 leaves)

F underneath; DUP by OPEN-BEADS.txt:88. Root cause M1. Ground truth: the independently checked real
123.RI offsets above, the Lotus Reference chapter for /Graph, and real command-minted output
fixtures. Extend the existing P4 owner with actual host command/output differentials plus locked
QEMU/Bochs execution and record-flair clips. Mutants making the command a visible or silent no-op
must fail. A navigable menu or a removed refusal is not acceptance.

Leaves: L199 /Graph/Type/Line; L200 /Graph/Type/Bar; L201 /Graph/Type/XY; L202
/Graph/Type/Stack-Bar; L203 /Graph/Type/Pie; L204 /Graph/X; L205 /Graph/A; L206 /Graph/B; L207
/Graph/C; L208 /Graph/D; L209 /Graph/E; L210 /Graph/F; L211 /Graph/Reset; L212 /Graph/View; L213
/Graph/Save; L214 /Graph/Options; L215 /Graph/Name/Use; L216 /Graph/Name/Create; L217
/Graph/Name/Delete; L218 /Graph/Name/Reset; L219 /Graph/Name/Table; L220 /Graph/Group/Columnwise;
L221 /Graph/Group/Rowwise.

### /Data: initech-1z3o (33 leaves)

F underneath; DUP by OPEN-BEADS.txt:88. Root cause M1. Ground truth: the independently checked real
123.RI offsets above, the Lotus Reference chapter for /Data, and real command-minted output
fixtures. Extend the existing P4 owner with actual host command/output differentials plus locked
QEMU/Bochs execution and record-flair clips. Mutants making the command a visible or silent no-op
must fail. A navigable menu or a removed refusal is not acceptance.

Leaves: L222 /Data/Fill; L223 /Data/Table/1; L224 /Data/Table/2; L225 /Data/Table/Reset; L226
/Data/Sort/Data-Range; L227 /Data/Sort/Primary-Key; L228 /Data/Sort/Secondary-Key; L229
/Data/Sort/Reset; L230 /Data/Sort/Go; L231 /Data/Query/Input; L232 /Data/Query/Criteria; L233
/Data/Query/Output; L234 /Data/Query/Find; L235 /Data/Query/Extract; L236 /Data/Query/Unique; L237
/Data/Query/Delete; L238 /Data/Query/Reset; L239 /Data/Distribution; L240 /Data/Matrix/Invert; L241
/Data/Matrix/Multiply; L242 /Data/Regression/X-Range; L243 /Data/Regression/Y-Range; L244
/Data/Regression/Output-Range; L245 /Data/Regression/Intercept/Compute; L246
/Data/Regression/Intercept/Zero; L247 /Data/Regression/Reset; L248 /Data/Regression/Go; L249
/Data/Parse/Format-Line/Create; L250 /Data/Parse/Format-Line/Edit; L251 /Data/Parse/Input-Column;
L252 /Data/Parse/Output-Range; L253 /Data/Parse/Reset; L254 /Data/Parse/Go.

Macro duplication within /Worksheet: L090/L091 Autoexec Enable/Disable and L112/L113/L114 Learn
Range/Cancel/Erase stay on initech-1z3o. Use real-Lotus macro/recorder key traces, worksheet byte
readback, host execution tests and mutation-proven QEMU/Bochs clips; do not add a second Worksheet
macro implementation owner.

L031 stays on initech-68iw.1. face_a.c:407-421 omits the active format descriptor; Lotus Reference
p. 10-4 (PDF 222) explicitly requires (F0)/(C2) style notation. Extend that existing descriptor
owner with independent status-line pixels and F0/C2 controls from shots/wide-format-fixed-0.png and
shots/wide-format-currency-2.png, plus a missing-descriptor mutant and a record-flair clip.

## New developer batches in user-harm order

BEADS.tsv is headerless with exactly five fields: title, type, priority (1..3), description, L-ids.
It contains only remaining D/F candidates, with one row per pickup. Start with input loss and
blocked capacity/calculation/power/navigation workflows, then missing capabilities, then qualified
compatibility/polish. Branch batches remain one feature owner even when their delivery needs
multiple sittings. The first sitting is bounded; completing it does not close still-unimplemented
leaves.

All GUI/app-window fixes require the relevant host oracle, an independent reference, a locked
emulator trace, and a short record-flair clip per Rule 14. QEMU/Bochs are named explicitly; add a
real-Lotus/period-emulator reference leg where requested and preserve any stronger subsystem
tri-emulator gate. Every proposed mutant must fail for the intended assertion before closure. No
golden may be derived from the values the implementation renders.

### B01 - Preserve pending entry text when decoding modified and lock keys (P2, bug)

Covers: L028, L034.

Root cause: os/i123/app/face_a.c:1074 and :860 treat every virtual key 0x46 as cancellation;
:1068-1144 checks ch==3 but does not decode Control-C from modifiers before appending printable c.
os/flair/event.c:491,530-547 supplies ordinary ASCII plus separate modifier flags and drops E0
provenance from the virtual key.

Ground truth: The explicit Ctrl-Break/Ctrl-C intent at face_a.c:860 and the authored
input-preservation expectation in REPORT.md L028/L034; this is not a claim that Ctrl-C is a
documented Lotus shortcut. spec/toolbox_gate.h Sec 6 defines the message/modifier contract.

Pickup: Decode cancellation using the actual event contract before printable insertion; preserve
unmodified Scroll Lock input and preserve already committed cells. If distinguishing Break requires
an ABI change, record it explicitly rather than guessing from scan code 0x46.

Acceptance oracle: Extend harness/proptest/test_i123_keys.c with raw cooked events for plain Scroll
Lock, Control-C, Control-Break, shifted C and ordinary c in LABEL/EDIT/range prompts. Replay
keyslook and edge_more on QEMU and Bochs, then save/extract WK1 and compare the retained text.
Mutants removing the modifier guard and treating Control-C as printable must fail for their named
reasons. Lock the GUI trace and record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L028, L034);
docs/audits/2026-10-23-i123/shots/scroll-lock-cancels.png;
docs/audits/2026-10-23-i123/evidence/keyslook-serial.log;
docs/audits/2026-10-23-i123/shots/ctrl-c-literal.png;
docs/audits/2026-10-23-i123/evidence/edge_more-serial.log;
docs/audits/2026-10-23-i123/evidence/entry-serial.log

### B02 - Reclaim erased default cells and reuse replacement label storage (P2, bug)

Covers: L010, L032.

Root cause: os/i123/app/face_a.c:756-758 erases then always reapplies the format, and
os/i123/core/sheet.c:388 allocates a BLANK even for the default format. sheet.c:250 allocates a
replacement blob before subtracting the old live bytes at :259; compaction at :207-209 still sees
both allocations.

Ground truth: The authored capacity-recovery/data-preservation expectations in REPORT.md L010/L032;
os/i123/core/sheet.c:308 and face_a.c:169-170 define the accepted label/store limits.
../lotus123-decomp/specs/file-formats/wk1-format.md Sec 4.1 says content-free default-format cells
are omitted, while nondefault format/protection needs BLANK records. Lotus Reference p. 10-3
requires erasing contents while preserving effective format/protection.

Pickup: Elide a BLANK only when absence represents exactly the same effective default format and
protection. Replace equal/smaller label blobs in place or transact them without requiring duplicate
live capacity; retain old contents on genuine failure. This owns allocation/recovery, not the
already tracked unexplained-exhaustion diagnostic in initech-vdgd.

Acceptance oracle: Add capacity and allocator cases to the sheet/Face A host oracles: 2048 cells
then erase ordinary A1 and enter A2049; preserve explicit-format/protection BLANKs; 135 labels of
240 characters then equal-size/shorter replacement and genuine overflow with old bytes intact.
Replay capacity and pool on QEMU/Bochs and independently parse CAPERA.WK1/POOLOUT.WK1. Mutants
restoring unconditional default BLANK allocation and allocate-before-reclaim must go red. Record a
record-flair clip.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L010, L032);
docs/audits/2026-10-23-i123/shots/capacity-after-erase.png;
docs/audits/2026-10-23-i123/evidence/capacity-serial.log;
docs/audits/2026-10-23-i123/evidence/extracted/capacity/CAPERA.json;
docs/audits/2026-10-23-i123/evidence/extracted/capacity/CAPERA.WK1;
docs/audits/2026-10-23-i123/shots/pool-replace-full.png;
docs/audits/2026-10-23-i123/shots/pool-old-preserved.png;
docs/audits/2026-10-23-i123/shots/pool-small-replace.png;
docs/audits/2026-10-23-i123/evidence/pool-serial.log;
docs/audits/2026-10-23-i123/evidence/POOL135.WK1;
docs/audits/2026-10-23-i123/evidence/extracted/pool/POOLOUT.json

### B03 - Format supported high decimal counts without a scaled-integer ceiling (P2, bug)

Covers: L030, L033.

Root cause: os/i123/core/fmt.c:245-248 and :784-787 multiply by 10^decimals then reject magnitudes
>=9e15. The display path emits stars independently of available width; eval.c:1166-1167 turns the
same fixed-text refusal into ERR for @STRING.

Ground truth: Lotus Reference pp. 10-4..10-6 (PDF 222..224) and p. 13-56 (PDF 344), retained in the
audit reference extractions: 0..15 decimal places, width-based display overflow, and @STRING(x,n).
Real rounding/precision bytes must come from Lotus rather than an artifact-derived expected string.

Pickup: Use a bounded decimal conversion that supports the documented decimal counts and separates
numeric conversion from field-width overflow. Preserve the existing currency/comma signs, percent
scaling, trailing reserved character and rounding rules; share the conversion where the rules agree.

Acceptance oracle: Extend test-i123-wk1-roundtrip and test-i123-formula with independent real-Lotus
goldens for the exact 1234.5678912345 cases, signs, zero, carry rounding, F/C/comma 12..15, percent
10..15 and @STRING. Replay formats_wide and edge_more on QEMU/Bochs; compare displayed text and
extracted formula/string caches separately. Mutants restoring each 9e15 guard must fail. Record a
record-flair clip.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L030, L033);
docs/audits/2026-10-23-i123/shots/wide-format-fixed-12.png;
docs/audits/2026-10-23-i123/shots/wide-format-fixed-13.png;
docs/audits/2026-10-23-i123/shots/wide-format-currency-13.png;
docs/audits/2026-10-23-i123/shots/wide-format-comma-13.png;
docs/audits/2026-10-23-i123/shots/wide-format-percent-11.png;
docs/audits/2026-10-23-i123/shots/wide-format-sci-15.png;
docs/audits/2026-10-23-i123/evidence/formats_wide-serial.log;
docs/audits/2026-10-23-i123/evidence/rendered-case-lines.json;
docs/audits/2026-10-23-i123/shots/empty-extra-00.png;
docs/audits/2026-10-23-i123/shots/empty-extra-01.png;
docs/audits/2026-10-23-i123/evidence/edge_more-serial.log;
docs/audits/2026-10-23-i123/evidence/extracted/edge_more/EMPTY.json

### B04 - Resume Finder power actions after an asynchronous worksheet Save (P2, bug)

Covers: L003.

Root cause: os/milton/kmain.c:5715-5723 asks Quit, services the immediate callback, then equates
still resident with cancellation. os/i123/app/face_a.c:1245-1247 has only opened the Save filename
prompt; successful write/exit happens later at :660-662. finder_power returns at kmain.c:5738-5739
and retains no pending continuation.

Ground truth: Inside Macintosh Vol VI PDF p. 345, Listing 6-8, as cited at kmain.c:5691-5704: the
normal Quit routine saves documents or returns userCanceledErr. The pending-Save continuation is an
authored native shutdown workflow requirement, not a DOS Lotus feature.

Pickup: Represent pending quit separately from explicit cancellation and resume the original Shut
Down/Restart action only after the app finishes successfully. Save failure/cancel keeps the desktop
and document open. Coordinate with initech-cqej for its decline/Bochs/clip tests; its title tracks
test coverage, not this asynchronous continuation defect, so L003 is not DUP.

Acceptance oracle: Extend the power/tenant host state-machine oracle and
test-flair-shutdown/test-flair-restart with Save -> filename -> successful exit -> one power
completion, Cancel, failed save, replacement cancellation, and no-app controls. Replay power twice
on QEMU and Bochs with independent WK1 extraction plus FLAIR-SHUTDOWN-SAFE/reset markers. A mutant
restoring still-resident==declined must fail the deferred-Save case. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L003);
docs/audits/2026-10-23-i123/shots/power-save-0-prompt.png;
docs/audits/2026-10-23-i123/shots/power-save-0-result.png;
docs/audits/2026-10-23-i123/shots/power-save-1-result.png;
docs/audits/2026-10-23-i123/evidence/power-serial.log

### B05 - Return prompts and ordinary error notices to their proper caller (P2, bug)

Covers: L018, L026.

Root cause: os/i123/app/face_a.c:864 sends every M_RANGE Escape through menu_mode even when A_GOTO
was entered directly from READY. At :868 Return dismisses M_NOTICE only when g_error is set, but
notice() at :578 sets g_notice instead.

Ground truth: Lotus Reference p. 1-5 (PDF 21) explicitly allows ESC or ENTER to clear an error;
chapter 1 GOTO/cancellation plus the authored caller-restoration expectation in REPORT.md L018.
Audit evidence/references/manual-pdf-021.txt contains the error-key rule.

Pickup: Retain the origin of direct GOTO versus slash-command range prompts and make all ordinary
error notices dismiss on Return as well as Escape. Do not change menu-level Escape/backtracking or
discard a pending input line on unrelated notices.

Acceptance oracle: Extend test_i123_keys.c and test_i123_range_menu.c with F5/Escape from READY,
nested menu range Escape, unsupported leaf/Return, invalid-GOTO/Return/Escape and saved-input
controls. Replay keyslook in QEMU/Bochs and assert mode/pointer/worksheet preservation. Mutants
routing all M_RANGE Escape to MENU and restoring the g_error-only Return guard must fail. Record a
record-flair clip.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L018, L026);
docs/audits/2026-10-23-i123/shots/goto-escape-menu.png;
docs/audits/2026-10-23-i123/evidence/keyslook-serial.log;
docs/audits/2026-10-23-i123/shots/unsupported-before-enter.png;
docs/audits/2026-10-23-i123/shots/unsupported-after-enter.png

### B06 - Render and hit-test the visible part of an oversized active column (P2, bug)

Covers: L008.

Root cause: os/i123/app/face_a.c:343 uses whole-column i123_cols_fitting (sheet.c:429-439); col_x at
:350-351 rejects even the first column when zero full columns fit. paint_border at :445 and
paint_pointer at :486-488 therefore omit heading and pointer although Set-Width accepts 1..240 at
:719-725.

Ground truth: The authored native-viewport usability requirement in REPORT.md L008 and the
implemented 1..240 width contract at sheet.c:398-402; Lotus Reference chapter 12 Column. Existing
native-window geometry is face_a.c:102-124. No requirement to shrink the supported width range is
inferred.

Pickup: Keep a partially visible first column in the view and consistently clip heading, cell,
pointer and hit-testing to the actual content rectangle. Retain full 240-character storage/display
width, and preserve navigation/resizing behavior.

Acceptance oracle: Extend test_i123_resize.c/test_i123_hit.c and the row renderer oracle with width
240 at normal and minimum window sizes, partial first columns, neighbor navigation and clicks.
Replay transfers width-240 and resize legs on QEMU/Bochs with visible heading/value/pointer
assertions. A mutant returning zero visible columns for an oversized first column must fail. Record
a record-flair clip.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L008);
docs/audits/2026-10-23-i123/shots/width-240.png;
docs/audits/2026-10-23-i123/shots/width-240-right.png;
docs/audits/2026-10-23-i123/evidence/transfers-serial.log

### B07 - Accept the documented leading dollar sign in numeric entry (P2, bug)

Covers: L004.

Root cause: os/i123/app/face_a.c:905-908 classifies $ as VALUE, but os/i123/core/fmt.c:653-668
accepts only an optional sign followed by digits/decimal point; commit at face_a.c:934-947 then
attempts formula parsing and refuses $12.34.

Ground truth: Lotus Reference p. 1-19 (PDF 35), audit evidence/references/manual-pdf-035.txt: a
leading dollar sign enters the number and currency display is a separate format choice.

Pickup: Recognize the documented typed-number currency prefix at the entry boundary without treating
absolute formula references such as +$A$1 as numeric literals. Preserve percent, exponent, sign and
invalid-input rules; capture sign/currency ordering edge cases from real Lotus before broadening the
grammar.

Acceptance oracle: Extend the numeric/Face A entry host oracles with $12.34 -> numeric 12.34,
separate currency formatting, absolute-reference controls and invalid embedded dollars. Replay entry
on QEMU/Bochs, save and independently decode the WK1 numeric record. A mutant rejecting the leading
dollar must fail. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L004);
docs/audits/2026-10-23-i123/shots/011-currency.png;
docs/audits/2026-10-23-i123/evidence/entry-serial.log

### B08 - Supply the existing NOW evaluator with the guest clock on recalculation (P2, bug)

Covers: L011.

Root cause: os/i123/core/recalc.c:92 initializes have_now=0; eval.c:912-914 correctly requires it.
os/i123/app/face_a.c:931,938,974-981 and :699-702 recalculate without supplying now/have_now
anywhere in the app. The existing clock service is os/apps/tbx/tbx.h:137-139, tbx_get_clock.

Ground truth: Lotus Reference p. 13-46 (PDF 334), @NOW current date/time serial refreshed at
recalculation; spec/i123/wk1_formula.h and the existing date serial rules, including the Lotus 1900
convention. This is an omitted input to an implemented evaluator, not evidence that the engine lacks
@NOW.

Pickup: Read the real guest clock through the existing gate into valid static/BSS storage, convert
to the Lotus serial and supply a coherent clock snapshot to every recalculation route and its nested
context. Clock failure must remain explicit, without fixed fake success values. Clock
rendering/configuration is separate from this formula input.

Acceptance oracle: Extend test-i123-formula and the Face A mock with injected guest dates/times, F9,
automatic edits, manual mode and nested evaluation. Replay functions on QEMU/Bochs with a locked
RTC, independently check saved @NOW numeric cache and its change after a known clock step. A mutant
omitting have_now or freezing the second snapshot must fail. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L011);
docs/audits/2026-10-23-i123/shots/func-25.png;
docs/audits/2026-10-23-i123/evidence/functions-serial.log;
docs/audits/2026-10-23-i123/evidence/extracted/functions/FUNCS.json

### B09 - Complete the remaining Worksheet branch and its Undo workflow (P2, feature)

Covers: L024, L036, L037, L038, L039, L040, L041, L042, L043, L044, L045, L046, L047, L048, L049, L050, L051, L052, L053, L054, L055, L056, L057, L058, L059, L060, L061, L062, L063, L064, L065, L066, L067, L068, L069, L070, L071, L072, L073, L074, L075, L076, L077, L078, L079, L080, L081, L082, L083, L084, L085, L086, L087, L088, L089, L092, L093, L094, L095, L096, L097, L098, L099, L100, L101, L102, L103, L104, L105, L106, L107, L108, L109, L110, L111.

Root cause: os/i123/app/face_a.c:817-849 implements only selected Worksheet formats/settings, erase,
widths and recalc; other leaves reach :851/685-687. International date/time explicitly refuses at
:824. No undo snapshot/history or Alt-F4 handler exists in key() at :1068-1146.
spec/i123/era_shortfalls.md I123-P3-01/05/08 documents these gaps; documentation is not deliberate
canon.

Ground truth: docs/plans/INITECH-123-plan.md Sec 4/7; Lotus Reference chapter 12 and p. 1-5 UNDO
(PDF 21), plus the real installed 123.RI strings at each offset in
docs/audits/2026-10-23-i123/evidence/menu-verified.json. spec/i123/menu_tree.golden is the local
menu inventory; independent 123.RI and real-Lotus operation goldens govern behavior.

Pickup: Keep one branch owner. First sitting: deliver safe structural Insert/Delete and the Undo
enable/disable + Alt-F4 recovery contract with reference adjustment; capture independent fixtures
before changes. The same branch backlog retains international formats/defaults, printer defaults,
directory/status/update, help/clock/add-in/beep settings, Zero, hide/display/range widths, titles,
split/sync windows, Status and Page. Coordinate macro Autoexec/Learn on initech-1z3o, existing
clock/line descriptors on initech-68iw.1, reached context help on initech-blx5, and Add-In execution
on B17. A first slice must not close the remaining listed leaves.

Leaves: L036 /Worksheet/Global/Format/Date/4 (Long Intn'l); L037 /Worksheet/Global/Format/Date/5
(Short Intn'l); L038 /Worksheet/Global/Format/Date/Time/1 (HH:MM:SS AM/PM); L039
/Worksheet/Global/Format/Date/Time/2 (HH:MM AM/PM); L040 /Worksheet/Global/Format/Date/Time/3 (Long
Intn'l); L041 /Worksheet/Global/Format/Date/Time/4 (Short Intn'l); L042
/Worksheet/Global/Default/Printer/Interface; L043 /Worksheet/Global/Default/Printer/AutoLF/Yes; L044
/Worksheet/Global/Default/Printer/AutoLF/No; L045 /Worksheet/Global/Default/Printer/Left; L046
/Worksheet/Global/Default/Printer/Right; L047 /Worksheet/Global/Default/Printer/Top; L048
/Worksheet/Global/Default/Printer/Bot; L049 /Worksheet/Global/Default/Printer/Pg-Length; L050
/Worksheet/Global/Default/Printer/Wait/No; L051 /Worksheet/Global/Default/Printer/Wait/Yes; L052
/Worksheet/Global/Default/Printer/Setup; L053 /Worksheet/Global/Default/Printer/Name; L054
/Worksheet/Global/Default/Directory; L055 /Worksheet/Global/Default/Status; L056
/Worksheet/Global/Default/Update; L057 /Worksheet/Global/Default/Other/International/Punctuation/A
(.,,); L058 /Worksheet/Global/Default/Other/International/Punctuation/B (,..); L059
/Worksheet/Global/Default/Other/International/Punctuation/C (.;,); L060
/Worksheet/Global/Default/Other/International/Punctuation/D (,;.); L061
/Worksheet/Global/Default/Other/International/Punctuation/E (., ); L062
/Worksheet/Global/Default/Other/International/Punctuation/F (,. ); L063
/Worksheet/Global/Default/Other/International/Punctuation/G (.; ); L064
/Worksheet/Global/Default/Other/International/Punctuation/H (,; ); L065
/Worksheet/Global/Default/Other/International/Currency/Prefix; L066
/Worksheet/Global/Default/Other/International/Currency/Suffix; L067
/Worksheet/Global/Default/Other/International/Date/A (MM/DD/YY); L068
/Worksheet/Global/Default/Other/International/Date/B (DD/MM/YY); L069
/Worksheet/Global/Default/Other/International/Date/C (DD.MM.YY); L070
/Worksheet/Global/Default/Other/International/Date/D (YY-MM-DD); L071
/Worksheet/Global/Default/Other/International/Time/A (HH:MM:SS); L072
/Worksheet/Global/Default/Other/International/Time/B (HH.MM.SS); L073
/Worksheet/Global/Default/Other/International/Time/C (HH,MM,SS); L074
/Worksheet/Global/Default/Other/International/Time/D (HHhMMmSSs); L075
/Worksheet/Global/Default/Other/International/Negative/Parentheses; L076
/Worksheet/Global/Default/Other/International/Negative/Sign; L077
/Worksheet/Global/Default/Other/Help/Instant; L078 /Worksheet/Global/Default/Other/Help/Removable;
L079 /Worksheet/Global/Default/Other/Clock/Standard; L080
/Worksheet/Global/Default/Other/Clock/International; L081
/Worksheet/Global/Default/Other/Clock/None; L082 /Worksheet/Global/Default/Other/Clock/Clock; L083
/Worksheet/Global/Default/Other/Clock/Filename; L084 /Worksheet/Global/Default/Other/Undo/Enable;
L085 /Worksheet/Global/Default/Other/Undo/Disable; L086 /Worksheet/Global/Default/Other/Beep/Enable;
L087 /Worksheet/Global/Default/Other/Beep/Disable; L088 /Worksheet/Global/Default/Other/Add-In/Set;
L089 /Worksheet/Global/Default/Other/Add-In/Cancel; L092 /Worksheet/Global/Zero; L093
/Worksheet/Insert/Column; L094 /Worksheet/Insert/Row; L095 /Worksheet/Delete/Column; L096
/Worksheet/Delete/Row; L097 /Worksheet/Column/Hide; L098 /Worksheet/Column/Display; L099
/Worksheet/Column/Column-Range/Set-Width; L100 /Worksheet/Column/Column-Range/Reset-Width; L101
/Worksheet/Titles/Both; L102 /Worksheet/Titles/Horizontal; L103 /Worksheet/Titles/Vertical; L104
/Worksheet/Titles/Clear; L105 /Worksheet/Window/Horizontal; L106 /Worksheet/Window/Vertical; L107
/Worksheet/Window/Sync; L108 /Worksheet/Window/Unsync; L109 /Worksheet/Window/Clear; L110
/Worksheet/Status; L111 /Worksheet/Page.

Acceptance oracle: Add/extend worksheet structural/Undo host oracles against real-Lotus before/after
WK1 and key/screen goldens; include formula/name/reference adjustment, protected cells, capacity
failure, undo disabled/enabled and restored bytes. For each remaining setting prove an actual
consumer and persistence where specified, not just accepted input. Preserve
test-i123-menu-tree/levels as inventory gates, then execute every listed leaf and compare its real
effect. QEMU/Bochs locked traces plus record-flair clips are required; mutants skipping each new
operation/consumer must go red.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L024, L036, L037, L038, L039, L040, L041,
L042, L043, L044, L045, L046, L047, L048, L049, L050, L051, L052, L053, L054, L055, L056, L057,
L058, L059, L060, L061, L062, L063, L064, L065, L066, L067, L068, L069, L070, L071, L072, L073,
L074, L075, L076, L077, L078, L079, L080, L081, L082, L083, L084, L085, L086, L087, L088, L089,
L092, L093, L094, L095, L096, L097, L098, L099, L100, L101, L102, L103, L104, L105, L106, L107,
L108, L109, L110, L111); docs/audits/2026-10-23-i123/shots/undo-absent.png;
docs/audits/2026-10-23-i123/evidence/keyslook-serial.log;
docs/audits/2026-10-23-i123/evidence/menu-verified.json;
docs/audits/2026-10-23-i123/evidence/menu-plan.json;
docs/audits/2026-10-23-i123/evidence/menus-serial.log;
docs/audits/2026-10-23-i123/shots/menu-014-selected.png;
docs/audits/2026-10-23-i123/shots/menu-014-result.png

### B10 - Complete inline editing and F4 reference-form cycling (P2, feature)

Covers: L005, L025.

Root cause: os/i123/app/face_a.c:1053-1054 explicitly defers the inline cursor; :1092 drops EDIT
Left/Right/Tab and :1138-1144 always appends. key() at :1068-1146 has no F4 handler or edit
insertion index. spec/i123/era_shortfalls.md I123-P3-08 also defers F4.

Ground truth: Lotus Reference pp. 1-22/1-23 (PDF 38/39) editing-key table and p. 1-11 (PDF 27) ABS
(F4) in POINT/EDIT; retained audit manual extractions. Capture the exact relative/absolute/mixed
cycling order from real Lotus.

Pickup: Maintain an insertion cursor and viewport with Left/Right/Home/End/Tab/Delete/Insert
behavior, and apply F4 to the active cell/range reference token. Coordinate capacity/prefix handling
with the L002 lane initech-pmw9; that data-loss finding is excluded here.

Acceptance oracle: Extend test_i123_entry.c/test_i123_keys.c with abcdef -> Left/X and Home/Y,
middle deletions, insert/overwrite, escape and long-line cursor visibility, then F4 on
single/range/mixed references. Use independent real-Lotus key traces and expected formula bytes.
QEMU/Bochs replay entry and keyslook, extract WK1, and record a record-flair clip. Mutants pinning
the cursor at the tail and ignoring F4 must fail.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L005, L025);
docs/audits/2026-10-23-i123/shots/017-edit-left.png;
docs/audits/2026-10-23-i123/shots/018-edit-home.png;
docs/audits/2026-10-23-i123/evidence/entry-serial.log;
docs/audits/2026-10-23-i123/shots/f4-absolute.png;
docs/audits/2026-10-23-i123/evidence/keyslook-serial.log

### B11 - Complete Copy destination tiling and interactive range anchoring (P2, feature)

Covers: L006, L007.

Root cause: os/i123/app/face_a.c:745 explicitly rejects a destination rectangle; range.c:177-184
only accepts one destination origin. command_key at face_a.c:869-878 stores a period as text and
arrows only move the pointer; there is no anchor/corner-selection state. spec/i123/era_shortfalls.md
I123-P3-07/08 records both unbuilt behaviors.

Ground truth: Lotus Reference /Copy pp. 3-4/3-5 (PDF 72/73) destination replication, and Specifying
a Range pp. 1-24/1-25 (PDF 40/41) period anchor and cancellation. Real-Lotus COPY01/WRAP01 establish
existing reference rules but do not by themselves prove destination tiling.

Pickup: One /Copy branch feature owner: implement destination rectangle replication and share
period-anchor selection across Copy/Move/GOTO/range prompts as applicable. Preserve overlap,
protection, relative/mixed/absolute reference behavior and transactional failure. There are no
additional unavailable /Copy or /Move menu-leaf findings in L036-L259.

Acceptance oracle: Extend test_i123_copy_move.c/test_i123_range_menu.c with independently minted
one-cell-to-D1..D3, tiled blocks, nonmultiples and boundary/refusal cases, plus
period/arrow/Backspace/Escape key traces. Replay transfers on QEMU/Bochs, compare saved
cells/formulas against real Lotus and record a record-flair clip. Mutants rejecting destination
ranges and treating period as ordinary text must fail.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L006, L007);
docs/audits/2026-10-23-i123/shots/copy-target-range-refused.png;
docs/audits/2026-10-23-i123/evidence/transfers-serial.log;
docs/audits/2026-10-23-i123/shots/point-dot-range.png;
docs/audits/2026-10-23-i123/shots/point-dot-result.png

### B12 - Complete named-range reassignment, formula display and F3 selection (P2, feature)

Covers: L009, L016, L020.

Root cause: os/i123/core/range.c:48-51 explicitly refuses changed bindings when any formula exists.
formula.c:259-263 resolves names to coordinates; fmt.c:625-630 decompiles without a sheet/name
table. face_a.c:869-878 has no F3 name-picker path. spec/i123/era_shortfalls.md I123-P3-06/08
acknowledges all three deferred capabilities.

Ground truth: Lotus Reference /Range Name pp. 10-14..10-18, especially p. 10-15 (PDF 233)
reassigning existing names, displayed-name substitution, F3 and relative-versus-absolute copy rules;
p. 1-24 (PDF 40) range prompts.

Pickup: Implement the reached named-range workflow over the existing NNAME table and add a bounded
selectable F3 menu. Capture real redefinition effects on pre-existing formulas first: do not assume
that every matching coordinate should be blindly rebound or that copied formulas retain the original
name spelling. Preserve deletion/reset/copy semantics and NNAME serialization.

Acceptance oracle: Extend name/range and Face A host oracles with rates B1, +rates*2, reassignment
to B2 with related/unrelated formulas, displayed names, name deletion, copied formulas, $name and F3
selection/cancel. Independent real-Lotus before/after bytes and screen transcripts decide semantics.
Replay transfers/keyslook on QEMU/Bochs and record a record-flair clip. Mutants restoring blanket
reassignment refusal, address-only display or a dead F3 must fail.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L009, L016, L020);
docs/audits/2026-10-23-i123/shots/name-redefine.png;
docs/audits/2026-10-23-i123/evidence/transfers-serial.log;
docs/audits/2026-10-23-i123/shots/f3-named-range.png;
docs/audits/2026-10-23-i123/evidence/keyslook-serial.log

### B13 - Complete the remaining Range menu branch (P2, feature)

Covers: L115, L116, L117, L118, L119, L120, L121, L122, L123, L124, L125, L126.

Root cause: os/i123/app/face_a.c:790-815 handles supported formats/labels/names/protection/transfers
but has no international date/time, Justify, Input or Search execution; unsupported leaves reach
:851/685-687. spec/i123/era_shortfalls.md I123-P3-04/05 enumerates these omissions.

Ground truth: Lotus Reference chapter 10 and docs/plans/INITECH-123-plan.md Sec 4; real installed
123.RI item/description offsets in docs/audits/2026-10-23-i123/evidence/menu-verified.json
independently identify all 12 leaves. International formats depend on actual Worksheet international
settings, not a hard-coded claim of parity.

Pickup: Keep one /Range feature batch. First sitting: reference-backed Search Find/Replace with safe
no-match/cancel and formula-versus-label targeting. Retain paragraph Justify, protected-cell Input
traversal and the six date/time format leaves on this owner. Coordinate shared international
settings/formatting with B09, and named-range capability with B12; neither dependency files these
leaves again.

Leaves: L115 /Range/Format/Date/4 (Long Intn'l); L116 /Range/Format/Date/5 (Short Intn'l); L117
/Range/Format/Date/Time/1 (HH:MM:SS AM/PM); L118 /Range/Format/Date/Time/2 (HH:MM AM/PM); L119
/Range/Format/Date/Time/3 (Long Intn'l); L120 /Range/Format/Date/Time/4 (Short Intn'l); L121
/Range/Justify; L122 /Range/Input; L123 /Range/Search/Formulas/Find; L124
/Range/Search/Formulas/Replace; L125 /Range/Search/Label; L126 /Range/Search/Both.

Acceptance oracle: Add independent real-Lotus host fixtures for each operation, including unchanged
worksheets on cancellation/refusal and precise replaced cells/formula text. Execute all 12 leaves
through locked QEMU/Bochs traces; compare saved WK1/text and screens with the reference, keeping
menu inventory gates green. Mutants skipping each operation, swapping formula/label targets or
ignoring locale/time must fail. Record record-flair clips.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L115, L116, L117, L118, L119, L120, L121,
L122, L123, L124, L125, L126); docs/audits/2026-10-23-i123/evidence/menu-verified.json;
docs/audits/2026-10-23-i123/evidence/menu-plan.json;
docs/audits/2026-10-23-i123/evidence/menus-serial.log;
docs/audits/2026-10-23-i123/shots/menu-156-selected.png;
docs/audits/2026-10-23-i123/shots/menu-156-result.png

### B14 - Complete the remaining File branch and worksheet file context (P2, feature)

Covers: L012, L013, L019, L029, L127, L128, L129, L130, L131, L132, L133, L134, L135, L136, L137, L138, L139, L140, L141, L142, L143, L144, L145, L146, L147, L148, L149, L150.

Root cause: os/i123/app/face_a.c:775 sends Backup to unsupported; :603 rejects spaces before
recognizing the password suffix. :325-333 and :880-887 only offer a typed filename; no tbx_file_find
consumer exists. :788 handles only Retrieve/Save and the other File leaves reach :851. eval.c:854
hard-codes an empty filename even though face_a.c:184,643,660 retains the saved/retrieved path.
spec/i123/era_shortfalls.md I123-P3-02 records the File execution gaps.

Ground truth: Lotus Reference chapter 5, especially Retrieve p. 5-19 (PDF 151), Backup p. 5-22 (PDF
154), password save pp. 5-22/5-23 (PDF 154/155), and @CELL filename p. 13-17 (PDF 305); real 123.RI
offsets in docs/audits/2026-10-23-i123/evidence/menu-verified.json. spec/toolbox_gate.h Sec 10/10a
and os/apps/tbx/tbx.h:130-139 provide native file enumeration/free-space/clock services.

Pickup: Keep one /File branch feature owner. First sitting: selectable Retrieve file list, safe
Backup preserving the previous WK1 in BAK, and the current canonical guest path supplied to @CELL
after successful Save/Retrieve. Coordinate replacement atomicity with the existing initech-mcep
lane, which owns L001; do not duplicate its safety fix. The branch remains open for password syntax
and real password encoding/readback, Combine/Xtract, file erasure/list/import/directory and Admin
reservation/link workflows. Capture format/security/locking rules before implementing them; a fake
password prompt or accepted-but-unused Admin setting is not completion.

Leaves: L127 /File/Combine/Copy/Entire-File; L128 /File/Combine/Copy/Named/Specified-Range; L129
/File/Combine/Add/Entire-File; L130 /File/Combine/Add/Named/Specified-Range; L131
/File/Combine/Subtract/Entire-File; L132 /File/Combine/Subtract/Named/Specified-Range; L133
/File/Xtract/Formulas; L134 /File/Xtract/Values; L135 /File/Erase/Worksheet; L136 /File/Erase/Print;
L137 /File/Erase/Graph; L138 /File/Erase/Other; L139 /File/List/Worksheet; L140 /File/List/Print;
L141 /File/List/Graph; L142 /File/List/Other; L143 /File/List/Linked; L144 /File/Import/Text; L145
/File/Import/Numbers; L146 /File/Directory; L147 /File/Admin/Reservation/Get; L148
/File/Admin/Reservation/Release; L149 /File/Admin/Table; L150 /File/Admin/Link-Refresh.

Acceptance oracle: Extend recording-gate/codec/host file-workflow oracles with real-Lotus
files/transcripts and independent WK1 decoding. Verify Backup old/new byte identity,
cancellation/no-space/fault preservation, file picker selection/cancel, filename after
Save/Retrieve/failed Save and password interoperability. For each listed File leaf execute its
actual effect; test destructive commands on copied media and compare mtools bytes. Replay
files/keyslook/menus on QEMU/Bochs and record record-flair clips. Mutants skipping Backup, file
listing, filename context or any delivered operation consumer must fail.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L012, L013, L019, L029, L127, L128, L129,
L130, L131, L132, L133, L134, L135, L136, L137, L138, L139, L140, L141, L142, L143, L144, L145,
L146, L147, L148, L149, L150); docs/audits/2026-10-23-i123/shots/backup-refused.png;
docs/audits/2026-10-23-i123/evidence/files-serial.log;
docs/audits/2026-10-23-i123/evidence/extracted/files/directory.txt;
docs/audits/2026-10-23-i123/shots/password-refused.png;
docs/audits/2026-10-23-i123/shots/cell-filename-saved.png;
docs/audits/2026-10-23-i123/evidence/keyslook-serial.log;
docs/audits/2026-10-23-i123/shots/menu-196-result.png;
docs/audits/2026-10-23-i123/evidence/menus-serial.log;
docs/audits/2026-10-23-i123/evidence/menu-verified.json;
docs/audits/2026-10-23-i123/evidence/menu-plan.json;
docs/audits/2026-10-23-i123/shots/menu-200-selected.png;
docs/audits/2026-10-23-i123/shots/menu-200-result.png

### B15 - Connect the worksheet vertical scroll bar to the retained view (P2, feature)

Covers: L027.

Root cause: os/i123/app/face_a.c:1293-1301 only recalculates geometry/follows the pointer on
updateEvt, and the entire face never calls TBX_SETSCROLL/TBX_GETSCROLL. spec/toolbox_gate.h:795-815
intentionally leaves standard bars disabled until a tenant supplies a vertical range;
tbxgate.c:1003-1031 already implements that service.

Ground truth: spec/toolbox_gate.h Sec 15 documents pixel units, 32767 maximum, value-change
updateEvt, and the deliberately disabled horizontal bar. REPORT.md L027 tests the vertical
arrow/track; its native-window usability expectation is authored, not a DOS Lotus mouse claim.

Pickup: Supply/read vertical range/value and translate between the limited pixel scroll contract and
8192 worksheet rows without truncating access to the lower rows. Keep keyboard follow, pointer,
scroll value and resize repaint coherent. This batch covers the observed vertical controls; the ABI
explicitly keeps horizontal scrolling disabled.

Acceptance oracle: Extend Face A resize/view host tests with arrow/page/thumb changes, keyboard GOTO
synchronization, resizing and rows near 8192; assert independent expected view rows and pointer
behavior. Replay keyslook vertical clicks/drag in QEMU/Bochs and record a record-flair clip. Mutants
omitting SetScroll or ignoring GetScroll on updateEvt must fail.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L027);
docs/audits/2026-10-23-i123/shots/scroll-before.png;
docs/audits/2026-10-23-i123/shots/scroll-arrow.png;
docs/audits/2026-10-23-i123/shots/scroll-thumb.png;
docs/audits/2026-10-23-i123/evidence/keyslook-serial.log

### B16 - Implement END status and block navigation (P2, feature)

Covers: L021.

Root cause: os/i123/app/face_a.c:1068-1097 has Home/arrows/page/Tab but no End latch or block scan,
and paint_status at :502-508 only paints CALC/CIRC. spec/i123/era_shortfalls.md I123-P3-08 does not
turn missing navigation into canon.

Ground truth: Lotus Reference pp. 1-9/1-10 (PDF 25/26) and status table p. 1-6 (PDF 22); End+Right
on the populated A1..F1 block reaches F1.

Pickup: Maintain the one-shot END navigation state and implement reference-backed jumps across
populated/blank boundaries on each axis, with cancellation and sheet-edge behavior.

Acceptance oracle: Extend test_i123_keys.c with contiguous blocks, gaps, empty starting cells, every
direction and A1/IV8192 boundaries using real-Lotus key/screen goldens. Replay keyslook in
QEMU/Bochs with END pixels and final coordinates. A mutant treating End+arrow as a single ordinary
step must fail. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L021);
docs/audits/2026-10-23-i123/shots/end-indicator.png;
docs/audits/2026-10-23-i123/shots/end-right.png;
docs/audits/2026-10-23-i123/evidence/keyslook-serial.log

### B17 - Provide the native Add-In branch lifecycle (P3, feature)

Covers: L256, L257, L258, L259.

Root cause: os/i123/app/face_a.c:772-851 dispatches no g_top==9 Add-In action, so
Attach/Detach/Invoke/Clear reach unsupported at :851/685-687. spec/i123/era_shortfalls.md I123-P3-03
explicitly notes no add-in loader/invocation, and docs/plans/INITECH-123-plan.md:186 includes the
menu in the product surface.

Ground truth: Lotus Reference Add-In chapter 2 and the real 123.RI lifecycle strings at audit
evidence/menu-verified.json offsets; ADR-0001 and CLAUDE.md:299-300 exclude executing genuine 16-bit
Lotus binaries. The no-v8086 constraint does not itself implement a native extension lifecycle.

Pickup: Keep one /Add-In feature batch and establish an explicit native extension contract with one
real attach/invoke/detach/clear fixture. Retain the binary-compatibility shortfall honestly; no
architecture change to run original 16-bit add-ins is authorized. Coordinate default key
assignments/settings with the Worksheet owner B09. A documented scope decision can bound later
parity, but the four current native entry points remain unbuilt until useful execution exists.

Leaves: L256 /Add-In/Attach; L257 /Add-In/Detach; L258 /Add-In/Invoke; L259 /Add-In/Clear.

Acceptance oracle: Add host lifecycle/ownership/error tests plus a native fixture producing an
independently specified visible/result change; absent/incompatible files must visibly refuse without
changing the sheet. Execute all four leaves on QEMU/Bochs and record a record-flair clip. Mutants
dropping attach state, leaving an invoked extension after detach, or making Invoke a success-only
stub must fail.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L256, L257, L258, L259);
docs/audits/2026-10-23-i123/evidence/menu-verified.json;
docs/audits/2026-10-23-i123/evidence/menu-plan.json;
docs/audits/2026-10-23-i123/evidence/menus-serial.log;
docs/audits/2026-10-23-i123/shots/menu-373-selected.png;
docs/audits/2026-10-23-i123/shots/menu-373-result.png

### B18 - Keep moved worksheet column-span metadata consistent with Lotus files (P3, bug)

Covers: L035.

Root cause: os/i123/core/range.c:188-191 clones metadata then erases cells; sheet.c:368-377 never
shrinks/rebuilds hid_first/hid_last. The transfer also uses direct i123_put_cell at range.c:209-210,
so some destination metadata does not pass through grow_extents. os/i123/fs/wk1.c:326-334 serializes
the retained arrays unchanged.

Ground truth: ../lotus123-decomp/goldens/minted/MOVE01B.WK1, MOVE01C.WK1, MOVE01D.WK1 and
mint/MOVE01.steps are command-minted ground truth.
../lotus123-decomp/specs/file-formats/wk1-format.md Sec 3.6 verifies the 0x0096 triples but labels
their general extent purpose inferred. Only byte-parity divergence is confirmed, not actual data
loss or load failure.

Pickup: Reconcile mutation bookkeeping with the independently observed span bytes after Move; first
capture empty-column, blank-gap, formatted-BLANK, copy and overwrite controls to establish exact
rules. Preserve opaque imported metadata when no relevant mutation occurs. Do not rewrite all
guessed spans in the codec or change the locked format silently.

Acceptance oracle: Extend harness/diff/wk1_diff/test_i123_copy_move.c beyond its present cell-only
comparison (cell_line/load_lines at :74-87, callers at :163-164) with independent 0x0096 comparison
against MOVE01B/C/D. Replay transfers on QEMU/Bochs and load native-produced files in real Lotus;
grade cells and metadata separately. A mutant preserving pre-Move spans must fail. Record a
record-flair clip; real-Lotus readback determines compatibility impact.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L035);
docs/audits/2026-10-23-i123/shots/MOVED.png;
docs/audits/2026-10-23-i123/evidence/transfers-serial.log;
docs/audits/2026-10-23-i123/evidence/move-metadata-comparison.json;
docs/audits/2026-10-23-i123/evidence/extracted/transfers/MOVED.WK1

### B19 - Explain invalid 8.3 names and no-space saves accurately (P3, bug)

Covers: L014, L017.

Root cause: os/i123/app/face_a.c:597-610 only rejects space/wildcards and appends an extension; it
does not validate 8.3 segment lengths before the gate. file_error at :585-588 maps everything except
missing file/path to File I/O error. The short/zero write branch at :650-658 also loses the
distinction when neither rc nor close is negative.

Ground truth: REPORT.md L014/L017 explicitly uses an authored actionable-error expectation; DOS 8.3
filename syntax and the Toolbox Sec 10 file-error contract are the underlying rules. Lotus Reference
p. 5-22 supplies the Save workflow. No Initech 123 shipped manual currently promises an alternate
message.

Pickup: Validate leaf syntax before mutation and preserve/report a typed no-space or
incomplete-write reason through the existing save stack, distinguishing bad names, access/device
errors and actual full media. Coordinate errors with atomic-save lane initech-mcep without re-owning
L001. Never clear dirty state or report Saved on refusal.

Acceptance oracle: Extend the recording file-gate host oracle with TOOLONGNAME, 12345678.1234,
bad-space/wildcards, valid qualified 8.3 controls, no-space, short write and device/access errors.
Replay files/full2 on QEMU/Bochs and verify messages, dirty state and stopped-image preservation.
Mutants collapsing bad-name/no-space into the generic branch must fail. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L014, L017);
docs/audits/2026-10-23-i123/shots/filename-0.png; docs/audits/2026-10-23-i123/shots/filename-5.png;
docs/audits/2026-10-23-i123/shots/filename-1.png;
docs/audits/2026-10-23-i123/evidence/files-serial.log;
docs/audits/2026-10-23-i123/shots/full2-full-new-refused.png;
docs/audits/2026-10-23-i123/evidence/full2-serial.log;
docs/audits/2026-10-23-i123/evidence/full2-before.txt

### B20 - Keep complete selected menu descriptions readable (P3, bug)

Covers: L015.

Root cause: os/i123/app/face_a.c:303 sends the description to panel_line; :281-283 pads/truncates to
SCR_COLS=76 and put_line at :261-262 clips to actual cols(). Menu item text scrolls at :300, but
selected descriptions have no scrolling/wrapping path.

Ground truth: The original installed 123.RI strings at the offsets in audit
evidence/menu-verified.json; all 762 item/description strings were independently re-read during
triage. The accepted native 76-column geometry is not a requirement to lose the last characters of a
selected description.

Pickup: Provide an actual readable continuation for the selected description at both normal and
resized widths; preserve complete resource strings and highlighted menu-item navigation. Do not
shorten Search or punctuation descriptions in the resource to fit.

Acceptance oracle: Extend the host menu renderer oracle with Range and punctuation B/D descriptions
at normal/small widths, expecting exact independent 123.RI bytes over the visible continuation.
Replay the three cited menu selections on QEMU/Bochs and record a record-flair clip. A mutant
clipping the final character must fail.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L015);
docs/audits/2026-10-23-i123/shots/menu-143-selected.png;
docs/audits/2026-10-23-i123/shots/menu-062-selected.png;
docs/audits/2026-10-23-i123/shots/menu-064-selected.png;
docs/audits/2026-10-23-i123/evidence/menus-serial.log;
docs/audits/2026-10-23-i123/evidence/menu-verified.json

### B21 - Expose and repaint CAPS and NUM worksheet status (P3, feature)

Covers: L022, L023.

Root cause: os/i123/app/face_a.c:502-508 paints only message/CALC/CIRC. os/flair/event.c:497-518
consumes Caps Lock as modifier state (no keyDown), :503-504 latches ALPHA_LOCK; the event cooker has
no Num Lock latch. Face A neither retains lock state nor obtains a NUM state, so a paint-only change
cannot cover both.

Ground truth: Lotus Reference status table pp. 1-5/1-6 (PDF 21/22) and NUM illustration cited in
REPORT.md L023; spec/toolbox_gate.h Sec 6 transports modifiers. Exact native indicator placement
must be captured/locked independently.

Pickup: Add a reliable guest lock-state observation/refresh path and render CAPS/NUM on/off without
losing CALC/CIRC/error/status text. Respect that Caps-only input may yield nullEvent and Num Lock
currently needs an explicit platform state contract; do not infer a toggle from arbitrary printable
events.

Acceptance oracle: Extend Event Manager and Face A host oracles with one physical toggle, held-key
repeat, release, second toggle, no unrelated keypress and repaint/resize; compare with independent
lock-state expectations. Replay keyslook on QEMU/Bochs with visible on/off states and record a
record-flair clip. Mutants omitting the lock latch or the status renderer must fail.

Audit evidence: docs/audits/2026-10-23-i123/REPORT.md (L022, L023);
docs/audits/2026-10-23-i123/shots/caps-indicator.png;
docs/audits/2026-10-23-i123/evidence/keyslook-serial.log;
docs/audits/2026-10-23-i123/shots/num-indicator.png

## Completion accounting and limits

- Findings: 259, L001-L259, each assigned exactly once.
- Verdicts: D=16; F=130; BY-DESIGN=1; DOC=0; DUP=110; LANE=2.
- New BEADS.tsv rows: 21 batches covering 146 findings; no lane/DUP/BY-DESIGN creates.
- Unavailable menu leaves: 224; F=114, DUP=109, BY-DESIGN=1.
- Branch totals: Worksheet 79, Range 12, File 24, Print 48, Graph 23, Data 33, System 1, Add-In 4. No unavailable Copy/Move leaves.
- New menu batches: /Worksheet, /Range, /File, /Add-In; /Copy is B11 with its observed range/tiling workflow. /Move L035 is a narrow implemented metadata defect.
- Tracked branches: /Print, /Graph, /Data plus five Worksheet macro leaves on initech-1z3o; /System is BY-DESIGN.
- Both data-loss findings are LANE; no new P1 row is needed. New priorities are P2 workflow/feature work and P3 compatibility/polish.
- Remaining uncertainties: exact real-Lotus name-redefinition/copy-display rules, high-precision rounding bytes, all unbuilt branch operation goldens, add-in native contract, and the general semantics/readback impact of record 0x0096.
- Validation performed: ASCII, five-field TSV schema, unique/exhaustive 259-row ownership, exact new-finding partition, branch counts, source/evidence path existence and independent 123.RI string-offset checks. Product oracles were not run.
