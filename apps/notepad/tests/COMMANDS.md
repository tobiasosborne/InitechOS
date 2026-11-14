# GM4 route ledger

Audit: notepad.c menu, key, mouse/track, turn, refresh and close dispatch;
both compile-time variants. Exact shipping rows/keys are literal test data,
checked by walking every installed row including contextual gray. No popup
or context-menu surface is present. Tests never infer expected strings from
the app's snapshot/store serializer. Native pixel fidelity is not claimed.

| Mode / object | Input routes | Observable contract / check |
|---|---|---|
| Both, clean/changed/damaged | File Close W / Quit Q / close box | Save and exit; failure keeps old bytes and editable state; damaged-store OK keeps damaged bytes |
| Note Pad, empty/selected/caret | Undo Z | Latest typing/deletion/Cut/Paste/Clear restores literal text, selection and dirty state; one-level toggle; no history on load/turn |
| Scrapbook, empty/one/three/full clips | Undo Z | Restore append/deletion, count/order/current page; Cut scrap unchanged; failed append preserves history |
| Both, valid range/whole TEXT clip | Cut X / Copy C / Clear | Exact literal clipboard/remaining text; no-scrap on Clear; contextual gray only without a valid range/clip |
| Both, TEXT/no-TEXT/invalid/oversized/full | Paste V | Literal append/replacement and line-end normalization; refusal keeps current store and prior undo |
| Note Pad | Select All A | Full editable range; removed completely from Scrapbook |
| Note Pad | Typing/autoKey, Backspace, arrows, click/shift-click/drag | Text/caret/selection change; cursor and menus break typing runs, repaint does not |
| Note Pad, pages 1..8 | Both corner buttons, boundary hits | Exact page label and independent saved record bytes; failed turn keeps page |
| Scrapbook, first/middle/last | Left/Right/Page Up/Page Down and scrollbar | Exact clip/index; boundaries clamp; history survives navigation |
| Long text | Scrollbar update; Scrapbook Up/Down | Viewport changes with text bytes unchanged |
| Both | Raw control chords, unused keys/removed Cmd-A | No text insertion or hidden command |

Host tests exercise the application side of this ledger; shared Menu Manager
key/mouse routing, title/scroll furniture and OK dismissal are shell-owned.
The smoke journey drives menu Undo and Clear, Ctrl-Z, Cut, Paste, Select All,
Copy and Close on QEMU/Bochs, with independent persisted-store readback.
The whole disk-wide discovery/painted inventory and complete shell-dialog
pointer sweep remain the shared factory acceptance responsibility.
