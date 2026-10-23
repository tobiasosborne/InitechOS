InitechWord audit evidence

Start with ../REPORT.md and ../NITS.tsv. findings.json is the same finding
catalog in machine-readable form. Reports are ASCII.

Evidence anchors:
- actions.jsonl: original timestamped actions, including composite helpers.
- sNN-primitive-actions.jsonl: keys/relative motion/buttons/waits, no duplicate
  type/click helpers. Each original screenshot action records serial_bytes.
- sNN-serial.log: full guest trace; sNN-session.json: command, PID, hashes/times.
- cleanup.jsonl: every QEMU was stopped with exit code zero.
- fixtures/, stress-fixtures/: original input bytes.
- sNN-extracted/: bytes copied with mtools only after QEMU stopped.
- sNN-bytes.json and directory.txt: sizes, hashes, body bytes/listings.
- wp5-independent-decoder.txt: the real-WP corpus decoder on 13 saved outputs.
- wp5-softpage-inputs-decoder.txt: the two rejected real WP input files.
- byte-validation.json: 36 explicit byte/header assertions, all passing.
- deliverable-validation.json: ID/TSV/evidence/ASCII/cleanup consistency.
- references/: actual WP captures and numbered original-manual text excerpts.
- source-contracts/: numbered read-only source/manual snapshots used for claims.
- verify-*.png/contact*.png: inspection aids, not substitutes for raw shots/.
- menu-pixel-verification.json: unchanged selected-file screens after no-op keys.
- SHA256.json: integrity manifest, excluding itself.

Preparation (from audit working directory):
  python3 evidence/prepare.py
  python3 evidence/prepare_full.py
  python3 evidence/prepare_stress.py
These touch only this audit's images/evidence. They read the supplied images
and the reference corpus. Existing prepared input images are retained for
exact replay, including their file timestamps.

Replay one session into a NEW destination:
  python3 evidence/replay.py s04 /tmp/iword-replay-s04
It copies the appropriate input images and runs one QEMU with no display,
486 CPU, 16 MB RAM and QMP stdio. Original UTC spacing is preserved, including
mouse press/release delays and time spent inspecting screenshots. It can
therefore take about as long as the original session. finally stops its QEMU.
Do not run multiple replay scripts at the same time on the shared machine.

Extract another stopped image copy:
  python3 evidence/extract.py images/s04-data.img /tmp/iword-s04-extracted
Never run extraction on an image while QEMU is using it.

Rebuild the report and check artifacts:
  python3 evidence/build_report.py
  python3 evidence/validate.py
The report builder is a fixed catalog of observed findings, not a source-only
bug detector. validate.py does not start an emulator or modify the repository.

Cautions about early screenshot names: the report's coverage section identifies
S02's failed first launch and the successful (misnamed) settings screenshot.
Some S01 copy-named shots actually show Move; S03 COPY.WP5 is the copy proof.
REPORT.md and findings.json are the reviewed conclusions.
