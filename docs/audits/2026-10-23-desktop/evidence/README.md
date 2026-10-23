# Evidence index

Start with ../REPORT.md and findings.json. All paths are relative to the audit
directory. Guest screenshots are the native 640x480 PNGs in ../shots/.

## Replay

Run one replay at a time from the audit directory:

    python3 evidence/replay_session.py keys01
    python3 evidence/replay_session.py trashmulti
    python3 evidence/replay_session.py full02
    python3 evidence/replay_session.py power
    python3 evidence/replay_session.py decline

This creates replay-prefixed private images, logs and shots. Default pacing
preserves recorded event times. --skip-idle is faster but changes long-hold
timing. The script consumes only the atomic rel/button/key/keyheld/shot events,
not the nested helper log records, so mouse gestures are not duplicated.

Short focused reproducers are reproduce_keyboard.py and
reproduce_multi_putaway.py. They use their own private image copies. Their
named output files can be overwritten on another invocation; archive outputs
first if preserving a second independent run matters.

prepare_fixtures.py rebuilds the many/deep/full input copies from the supplied
flair_data.img. Existing fixture images preserve the original timestamps and
hashes. prepare_declining_fixture.py uses the archived ctenant_decline.exe;
decline-manifest.txt identifies its bytes. No repository build is needed.

## Session map

| Session | Main purpose |
| --- | --- |
| base | Initial desktop, menus, CTENANT drawing, built-ins, Apple About, egg |
| edge01 | 82 icons, scroll/resize, replacement, purge lifetime, lock, Put Away, 25 windows |
| full01 | Full-disk boot alert; later attempted actions were blocked by that alert |
| full02 | Corrected full-disk recovery and error-message tests |
| decline | Test-only tenant vetoes Restart, Shut Down and Exit to InitechDOS |
| keys01 | Stale keyboard selection, horizontal reveal, cycle and selection gestures |
| menus | Executable command Open, resident-app limit, tenant menus, views, long hold, window boxes |
| trashmulti | Collision names and multiple-selection Put Away defect; stopped before recovery |
| trashresume | Rebooted single-item restoration and locked-descendant purge |
| power | Consenting tenant, reset, safe shutdown, Restart button, DOS exit |
| keyboard-repro | Independent minimal keyboard reproduction |
| inputfinal | Small overflow, endpoint thumb click, collapse, drag typing, click timing; harness token failure |
| inputfinish | HMP modifier sweep; subsequent intended built-in shots invalid, see harness-notes.txt |
| manytrash | 83-object recursive purge and Cancel drag-off tracking |
| directinputs | Explicit QMP key make/break sweep, valid built-in menus, upper menus, screen corners |

The *-session.json files contain exact emulator command lines, process IDs,
image hashes and exit records. actions.jsonl contains UTC-stamped input actions.
cleanup.jsonl is the independent driver cleanup journal. The short driver PID
numbers are those in the command execution environment; they are not claims
about other auditors' host process IDs.

## Data and references

The *-fsck.txt files are non-mutating fsck.fat -n results from stopped copies.
The extracted .TXT/.BAT/.DB/.exe files and directory inventories retain byte
evidence. verification-metrics.json includes important payload hex/hashes,
the unchanged keyboard-window crop, and the observed window-slot set.

references/ contains the local Mac capture/spec copies and the shipped manual
excerpt/source. measure_look.py regenerates look-metrics.json from those native
captures and the current guest PNGs. prior-report-index.json records the older
audit reports and their finding headings.

validate_evidence.py checks ASCII/TSV structure, cited file existence, PNGs,
clean emulator exits, serial crash/drop markers, and stopped data-disk FAT
consistency. Run it only when all audit-owned emulators have stopped.
