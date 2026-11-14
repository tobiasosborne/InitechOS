# GM4 command audit

Scope: calcda.c event switch; calc_core.c key map; calc_view.c hit test.
The test's literal manifest is authoritative; it walks the actual installed
menu in full, including disabled rows. A changed manifest must be reviewed.

| Mode / object | Route | Independent effect / check |
|---|---|---|
| Documentless | File Close W, Quit Q; close box | Exit 0; host exit contract and booted desktop journey |
| Any readout | Edit Copy, pointer | Exact TEXT 20 / 1.5 / 1E-3 / infinity; arithmetic unchanged; error notice on failed transport |
| Pending 2+3 | Edit Clear, pointer | 0 then 4= gives 4; scrap unchanged |
| Entry / pending / result | 0..9 . + - * / = C, keyboard and each button | Literal outcomes in route test; full real-Mac vectors and independent arithmetic in test_calcda |
| Any | Z/X/V control chords | Absent from resource, no effect |
| Any | Update, autoKey | Readout paint / same key map; no separate commands |

No text selection, file dialogs, editable fields, contextual menus or other
command buttons. The notice's OK is a shell-owned dismissal. Native menu
shortcuts are exported from the resource and dispatched by the shell; they
are not raw control-character text entry. Pointer menu routes are exercised
on both emulators by tests/smoke/journey.sh.
