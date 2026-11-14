# GM4 route ledger

Audit: itext.c menu, guarded/run_after, key, mouse/track, SF reply and alert
switches; TextEdit exposes one plain-text object. No contextual/popup menu.
The literal row/key manifest is tested for equality against SETMBAR, including
contextual gray; no expected manifest is generated from product arrays.

| Mode / object | Routes | Independent observable outcomes |
|---|---|---|
| Clean/dirty/untitled/named | New N, Open O, Close W, Save S, Save As, Quit Q; close box | Empty document / literal opened or saved bytes / exit. Unsaved Save/Cancel/Don't Save tested separately; SF reply/cancel preserves or changes title/document as specified |
| Empty/selected/dirty/full | Undo Z, Cut X, Copy C, Paste V, Clear, Select All A | Literal before/after strings, scrap bytes, selection/dirty state; capacity error conserves document and history |
| Text/caret/range | Printable and Return/Backspace/autoKey, four arrows, click/shift-click/drag | Exact text, caret/selection and viewport. Typed runs undo together; navigation breaks runs |
| Long/short text | Scrollbar update, activation/repaint | Literal independent extent/caret bounds and unchanged text/history |
| Named document, I/O errors | Open/save and dialogs | Existing document and original file preserved; independent disk-backed load test reads files outside app |
| All | Removed Print P, other raw control chords | No prompt, text insertion or hidden command |

Source-side outcomes are exercised by test_itext_route and test_itext_load.
Shared shell modal buttons, native shortcut mapping and pointer furniture
belong to SDK/FLAIR; host tests drive their actual app callback identities,
not the shell's pixels. The app smoke drives the pruned menu's Quit with the
pointer and the standard Save/Close routes on both emulators. Complete
boot-visible inventory/button coverage is shared factory acceptance work.
