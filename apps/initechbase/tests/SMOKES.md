# Deferred emulator gates

Historical gate names refer to legacy-final. None is run or ported here.
Default image: build/tracer_secondary.img, with the real application EXE/OVL
installed on a private second FAT volume or boot overlay; serial trace must
complete SHELL-DONE/quit-after without triple fault. SAMIR_LIST_IMG is the
captured database listing data image; SAMIRBOOT_CAPTURE_IMG carries the
period screen capture. FLAIR desktop legs use FLAIRTENANTS_INTERACTIVE_IMG.
Bochs legs replay their explicit key traces with the same corpus inputs.

DML/FNS matrices replay the named captured programs through the real shell,
compare alternate/printer output and read DBF/MEM/NDX files back independently.
Audit/use/list-layout compare dot prompt, scopes, input/error handling and real
mint 005 transcripts. Report checks mint 004 pages/totals and abbreviated forms.
Core/safety/lifetime/full tests check overflow bounds, repeated ownership and
disk-full atomic failure. Overlay tests exercise missing/stale/corrupt files,
retry/abort, relocation and retained-pointer/live-slot guards. Screen/form/dot/
cursor/console gates use screen-mint and W4 KEYS/DUMPS, exact char/attribute/
cursor captures and pixels; the status-pending leg remains known unfinished.
Canon Y2K/salami replay their recorded programs and intentional period bugs.
Boot/write/FLAIR integration checks launch/exit, screen output and disk writes.
Every mutant suffix reruns the same trace with its named wrong-code branch.

| Legacy gate | Image / trace family |
|---|---|
| `test-flair-samir-suspend-bochs` | interactive FLAIR tenant image / launch keys |
| `test-interp-do-file-emu` | secondary tracer / named host program replay |
| `test-samir-abbrev-emu` | secondary tracer / named host program replay |
| `test-samir-audit-emu` | secondary tracer / named host program replay |
| `test-samir-audit-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-bochs-keys` | Bochs key trace / secondary tracer |
| `test-samir-bochs-keys-mutant` | Bochs key trace / secondary tracer |
| `test-samir-boot` | SAMIRBOOT_CAPTURE_IMG + SAMIR_LIST_IMG / listing capture and mutant trace |
| `test-samir-boot-bochs` | SAMIRBOOT_CAPTURE_IMG + SAMIR_LIST_IMG / listing capture and mutant trace |
| `test-samir-boot-bochs-mutant` | SAMIRBOOT_CAPTURE_IMG + SAMIR_LIST_IMG / listing capture and mutant trace |
| `test-samir-boot-mutant` | SAMIRBOOT_CAPTURE_IMG + SAMIR_LIST_IMG / listing capture and mutant trace |
| `test-samir-boot-stream-mutant` | SAMIRBOOT_CAPTURE_IMG + SAMIR_LIST_IMG / listing capture and mutant trace |
| `test-samir-canon-salami` | secondary tracer / named host program replay |
| `test-samir-canon-salami-mutant` | secondary tracer / named host program replay |
| `test-samir-canon-y2k` | secondary tracer / named host program replay |
| `test-samir-canon-y2k-mutant` | secondary tracer / named host program replay |
| `test-samir-console-emu` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-console-emu-mutant` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-console-review-color` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-console-review-color-state` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-console-review-continuation` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-console-review-controls` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-console-review-keys` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-console-review-mutant` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-core-emu` | secondary tracer / named host program replay |
| `test-samir-core-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-core-emu-mutants` | secondary tracer / named host program replay |
| `test-samir-ctl-emu` | secondary tracer / named host program replay |
| `test-samir-ctl-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-cursor25` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-cursor25-mutant` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-dml-abbrev-emu` | secondary tracer / named host program replay |
| `test-samir-dml-abbrev-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-abbrev-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-abbrscope-emu` | secondary tracer / named host program replay |
| `test-samir-dml-abbrscope-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-abbrscope-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-append-emu` | secondary tracer / named host program replay |
| `test-samir-dml-append-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-append-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-clear-emu` | secondary tracer / named host program replay |
| `test-samir-dml-clear-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-clear-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-close-emu` | secondary tracer / named host program replay |
| `test-samir-dml-close-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-close-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-copyfile-emu` | secondary tracer / named host program replay |
| `test-samir-dml-copyfile-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-copyfile-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-emu` | secondary tracer / named host program replay |
| `test-samir-dml-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-emu-mutants` | secondary tracer / named host program replay |
| `test-samir-dml-erase-emu` | secondary tracer / named host program replay |
| `test-samir-dml-erase-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-erase-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-index-emu` | secondary tracer / named host program replay |
| `test-samir-dml-index-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-index-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-label-emu` | secondary tracer / named host program replay |
| `test-samir-dml-label-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-label-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-macro-emu` | secondary tracer / named host program replay |
| `test-samir-dml-macro-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-macro-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-memory-emu` | secondary tracer / named host program replay |
| `test-samir-dml-memory-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-memory-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-reindex-emu` | secondary tracer / named host program replay |
| `test-samir-dml-reindex-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-reindex-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-rename-emu` | secondary tracer / named host program replay |
| `test-samir-dml-rename-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-rename-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-restore-emu` | secondary tracer / named host program replay |
| `test-samir-dml-restore-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-restore-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-run-emu` | secondary tracer / named host program replay |
| `test-samir-dml-run-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-run-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-save-emu` | secondary tracer / named host program replay |
| `test-samir-dml-save-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-save-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-sort-emu` | secondary tracer / named host program replay |
| `test-samir-dml-sort-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-sort-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-status-emu` | secondary tracer / named host program replay |
| `test-samir-dml-status-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-status-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-statusinto-emu` | secondary tracer / named host program replay |
| `test-samir-dml-statusinto-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-statusinto-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-total-emu` | secondary tracer / named host program replay |
| `test-samir-dml-total-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-total-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-dml-type-emu` | secondary tracer / named host program replay |
| `test-samir-dml-type-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-dml-type-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-do-file-emu` | secondary tracer / named host program replay |
| `test-samir-dotprompt` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-dotprompt-mutant` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-fns-alias-emu` | secondary tracer / named host program replay |
| `test-samir-fns-alias-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-alternate-emu` | secondary tracer / named host program replay |
| `test-samir-fns-alternate-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-argorder-emu` | secondary tracer / named host program replay |
| `test-samir-fns-argorder-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-argpicture-emu` | secondary tracer / named host program replay |
| `test-samir-fns-argpicture-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-col-emu` | secondary tracer / named host program replay |
| `test-samir-fns-col-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-console-emu` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-fns-console-emu-mutant` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-fns-diskspace-emu` | secondary tracer / named host program replay |
| `test-samir-fns-diskspace-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-division-emu` | secondary tracer / named host program replay |
| `test-samir-fns-division-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-emu` | secondary tracer / named host program replay |
| `test-samir-fns-emu-mutants` | secondary tracer / named host program replay |
| `test-samir-fns-errors-emu` | secondary tracer / named host program replay |
| `test-samir-fns-errors-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-exp-emu` | secondary tracer / named host program replay |
| `test-samir-fns-exp-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-exppicture-emu` | secondary tracer / named host program replay |
| `test-samir-fns-exppicture-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-exprcol-emu` | secondary tracer / named host program replay |
| `test-samir-fns-exprcol-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-fklabel-emu` | secondary tracer / named host program replay |
| `test-samir-fns-fklabel-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-getenv-emu` | secondary tracer / named host program replay |
| `test-samir-fns-getenv-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-index-emu` | secondary tracer / named host program replay |
| `test-samir-fns-index-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-inkey-emu` | secondary tracer / named host program replay |
| `test-samir-fns-inkey-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-iscolor-emu` | secondary tracer / named host program replay |
| `test-samir-fns-iscolor-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-limitpic-emu` | secondary tracer / named host program replay |
| `test-samir-fns-limitpic-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-listoff-emu` | secondary tracer / named host program replay |
| `test-samir-fns-listoff-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-log-emu` | secondary tracer / named host program replay |
| `test-samir-fns-log-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-logdoarg-emu` | secondary tracer / named host program replay |
| `test-samir-fns-logdoarg-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-logguards-emu` | secondary tracer / named host program replay |
| `test-samir-fns-logguards-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-lupdate-emu` | secondary tracer / named host program replay |
| `test-samir-fns-lupdate-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-matrixb-emu` | secondary tracer / named host program replay |
| `test-samir-fns-matrixb-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-ndx-emu` | secondary tracer / named host program replay |
| `test-samir-fns-ndx-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-ndx-handler-depth-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-ndx-handler-name-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-ndxdisplay-emu` | secondary tracer / named host program replay |
| `test-samir-fns-ndxdisplay-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-ndxdoarg-emu` | secondary tracer / named host program replay |
| `test-samir-fns-ndxdoarg-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-numconvert-emu` | secondary tracer / named host program replay |
| `test-samir-fns-numconvert-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-numlist-emu` | secondary tracer / named host program replay |
| `test-samir-fns-numlist-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-os-emu` | secondary tracer / named host program replay |
| `test-samir-fns-os-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-pcol-emu` | secondary tracer / named host program replay |
| `test-samir-fns-pcol-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-prow-emu` | secondary tracer / named host program replay |
| `test-samir-fns-prow-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-readkey-emu` | secondary tracer / named host program replay |
| `test-samir-fns-readkey-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-recsize-emu` | secondary tracer / named host program replay |
| `test-samir-fns-recsize-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-retryconfirm-emu` | secondary tracer / named host program replay |
| `test-samir-fns-retryconfirm-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-returnarg-emu` | secondary tracer / named host program replay |
| `test-samir-fns-returnarg-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-row-emu` | secondary tracer / named host program replay |
| `test-samir-fns-row-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-sqrt-emu` | secondary tracer / named host program replay |
| `test-samir-fns-sqrt-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-sqrtdoarg-emu` | secondary tracer / named host program replay |
| `test-samir-fns-sqrtdoarg-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-sqrtpass-emu` | secondary tracer / named host program replay |
| `test-samir-fns-sqrtpass-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-struct-emu` | secondary tracer / named host program replay |
| `test-samir-fns-struct-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-time-emu` | secondary tracer / named host program replay |
| `test-samir-fns-time-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-type-emu` | secondary tracer / named host program replay |
| `test-samir-fns-type-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-unknown-emu` | secondary tracer / named host program replay |
| `test-samir-fns-unknown-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-fns-version-emu` | secondary tracer / named host program replay |
| `test-samir-fns-version-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-forms-emu` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-forms-emu-mutant` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-full-emu` | secondary tracer / named host program replay |
| `test-samir-full-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-input-emu` | secondary tracer / named host program replay |
| `test-samir-input-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-lifetime-desktop` | interactive FLAIR tenant image / launch keys |
| `test-samir-lifetime-desktop-mutant` | interactive FLAIR tenant image / launch keys |
| `test-samir-lifetime-emu` | secondary tracer / named host program replay |
| `test-samir-lifetime-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-list-layout-emu` | secondary tracer / named host program replay |
| `test-samir-list-layout-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-memwidth-emu` | secondary tracer / named host program replay |
| `test-samir-memwidth-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-messages-emu` | secondary tracer / named host program replay |
| `test-samir-messages-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-numeric-format-emu` | secondary tracer / named host program replay |
| `test-samir-numeric-format-emu-fraction-mutant` | secondary tracer / named host program replay |
| `test-samir-numeric-format-emu-left-mutant` | secondary tracer / named host program replay |
| `test-samir-ovl-emu` | secondary tracer / named host program replay |
| `test-samir-ovl-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-ovl-guard-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-ovl-swap-emu` | secondary tracer / named host program replay |
| `test-samir-ovl-swap-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-print-emu` | secondary tracer / named host program replay |
| `test-samir-print-emu-bochs` | Bochs key trace / secondary tracer |
| `test-samir-print-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-report-emu` | secondary tracer / named host program replay |
| `test-samir-report-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-safety-emu` | secondary tracer / named host program replay |
| `test-samir-safety-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-size-emu` | secondary tracer / named host program replay |
| `test-samir-size-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-status-emu` | secondary tracer / named host program replay |
| `test-samir-status-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-status-off-emu` | secondary tracer / named host program replay |
| `test-samir-status-screen-pending` | secondary tracer / captured screen KEYS/DUMPS |
| `test-samir-undefined-emu` | secondary tracer / named host program replay |
| `test-samir-undefined-emu-mutant` | secondary tracer / named host program replay |
| `test-samir-write` | secondary tracer / named host program replay |
| `test-samir-write-mutant` | secondary tracer / named host program replay |
