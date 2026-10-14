# Initech 123 P3 shortfalls (initech-uqah)

New ledger, introduced deliberately for plan section 7 item 4. The menu words
and descriptions remain R2.2's; the response `Command not implemented` is an
AUTHORED, visible refusal, not a claim that Lotus used that sentence. Esc
returns to READY after a refusal. No unavailable command silently succeeds.

| ID | Surface | Current behavior and remaining work | Reference |
|---|---|---|---|
| I123-P3-01 | Worksheet structural/settings commands | Insert, Delete, Titles, Window, Status, Page, Learn; Column Hide/Display/Column-Range; Global Default/Zero refuse at their leaf. Their included menus remain navigable. Column Set-Width/Reset-Width, global width/format/label prefix/protection and all recalculation settings work. | Plan sections 4, 6 P3/P4; R2.2 Reference chapter 12; RI 0xba1..0x279d. |
| I123-P3-02 | Print, Data, Graph, File extensions | Feature execution remains P4. File Retrieve/Save work; other File leaves refuse. Print Printer/File and settings menus navigate but printing refuses. Data menus navigate but commands refuse. Graph's listed first-level items, Type, Name, Group navigate; Options/Reset subtrees and rendering are deferred. | Plan section 7 item 5; Reference chapters 4/5/6/8. |
| I123-P3-03 | System, Add-In | System cannot run real 16-bit DOS programs under ADR-0001. Add-In menus navigate but loading/invocation refuses; binary-compatible Lotus add-ins are unavailable. | Plan section 4, ADR-0013; RI 0x277, 0x2af, 0x3a0..0x58a. |
| I123-P3-04 | Range Justify/Input/Search | Justify's paragraph reflow, protected-cell input traversal and Find/Replace execution defer. Search menus navigate; its Formulas branch exposes the Find/Replace choices while parameter entry and execution defer. | Reference chapter 10; RI 0x28bb..0x2eef. |
| I123-P3-05 | Range/global Format | Fixed, Sci, Currency, Comma, General, +/-, Percent, Date 1/2/3, Text, Hidden and Reset work. International Date 4/5 and Time are navigable and refuse; international configuration/clock is deferred. Formula Text displays within its column without spilling. | RI 0x58b..0xa34; Reference chapter 10; existing WK1 format header. |
| I123-P3-06 | Range names | NNAME table persists byte-exact, Create/Delete/Labels/Reset/Table work, formulas resolve NAME (relative) and $NAME (absolute). The formula panel continues to show resolved addresses; automatic substitution of names into displayed formulas and retargeting already compiled formulas when redefining a name are deferred. Redefinition to different coordinates refuses while formulas exist, preventing silently stale bindings. Existing formula addresses survive deletion/reset. Legacy NAME opcode 0x000b remains opaque; the corpus uses NNAME 0x0047. | MINT02.WK1 header 0x86/body 0x8a; Reference pp. 10-14..18 (PDF 232..236), 3-3 (PDF 71). |
| I123-P3-07 | Copy/Move extent and evidence | TO accepts one upper-left cell; replicated/tiling TO ranges defer and visibly refuse. Overlap uses a deterministic snapshot (authored; Lotus warns overlap can be surprising). Copy/Move reference rules pass the model and existing-mint self-transfer baselines. Command-minted differential replay remains UNVERIFIED: this corpus has zero /Copy or /Move mint commands. The required test-i123-copy-move gate fails loudly until acquisition and replay are completed. | Reference pp. 3-4/5 (PDF 72/73), 7-1/2 (PDF 191/192); corpus mint/*.steps. |
| I123-P3-08 | Function keys, range picking, macros | F2 editing, F5 GoTo and F9 recalculation work. F3/F4/F6/F7/F8/F10 and macros remain deferred. Range prompts accept typed rectangles/names and arrow-picked single cells; multi-cell interactive selection defers. | Plan sections 4/6 P3/P4; Reference chapter 1. |
| I123-P3-09 | Classic viewport | The native 77-column window is three columns narrower than DOS. Long menu lines scroll to keep the highlighted item visible; unhighlighted trailing text may be clipped. Every complete item/description is retained in the resource and independently graded. | Plan section 3.3; face_a.c geometry; R2.2 80-column screens. |

Capacity is an explicit implementation limit: 2048 populated cells, a 32 KiB
cell byte pool per bank, 4096 bytes of peripheral/name records. Operations
build a candidate bank; failures leave the current sheet and its dirty flag
intact. Menus live in authored APPS/123.MNU, separate from the 64 KiB-capped
executable. Missing/malformed menu resources refuse startup with EXIT(1).

Bochs and the Rule-14 i123_menu clip are PM acceptance work on main; this
lane does not claim them. No WK1 locked spec header was changed.
