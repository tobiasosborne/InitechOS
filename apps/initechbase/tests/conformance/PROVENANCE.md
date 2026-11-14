# Reference provenance

Product under study: InitechOS `09d52a32173911debc13035096135143fefb4b2c`.
All `reference/Cnn/RESULT.OUT` files were captured from real dBASE III PLUS 1.1
running on a copy of genuine DOS 3.30 OEM boot media. The runtime files and
boot-media SHA-256 hashes are in `reference-runtime.sha256`; no runtime binaries
or DOS media are redistributed here. No InitechBase output is an expected value.
`reference/SHA256SUMS` pins the actual alternate files and mtools readbacks.

Capture host: QEMU 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18), CPU 486, 8 MiB RAM,
no network, fixed RTC 1992-03-02 09:30 UTC, virtual clock. One fresh boot and one
fresh 32 MiB FAT16 disk per case. CONFIG.SYS FILES=40, BUFFERS=20; CONFIG.DB
STATUS=OFF, BELL=OFF. `capture.sh` preserves these configurations and every
command; `cases/Cnn` contains the exact mounted files.

Independent capture driver: `goldens/dataloss/drive.c`, unchanged, SHA-256
`6e993d7bb26340ad4ce35155f3e3b14649e551957da97bcb6243dd80d1bfa026`.
It waits for the monitor prompt after each memory/screen dump, waits for the
DOS CAPTURE-DONE marker after DBASE exits, and dumps VGA. `result.vga` and
`result.txt` accompany every completed capture. `exit-status` is the driver
status. Raw DOS alternate output and readback bytes are authoritative; the
end-of-run VGA corroborates completion rather than every preceding screen.

The capture operation was `sh tests/conformance/capture.sh ABSOLUTE_EMPTY_DIR`
from the component, with optional CASES selection. Retention used `retain.sh`,
which compared every input against `before/`, required driver success and a
fresh END marker, then retained only actual disk readback. C01-C04 and C35 were
recaptured after correcting the test input; no superseded captures are used.
All 36 retained inputs match `INPUTS.sha256`. Replaying a capture must use a
new output directory. Host observations use `w2_host.c` and the actual unmodified
interpreter/codecs, through a separate runner; five-second per-case watchdog.

Prior corpus examined: `goldens/README.md`, CORPORA.lock's dBASE entries,
component captured-hashes and independent readers, the pristine DBF/DBT/NDX
samples, and `tests/xbase_prog_diff/do-file-mint/depth/GOLD.OUT`. The latter
provides the independent procedure-library depth control (19 nested routine
entries then #103), distinct from C33's disk-program handle exhaustion.

Geometry and probe rationale came from the local independent corpus documents
listed below. These describe the foreign format; they do not supply expected
error handling for malformed files. Fresh real III+ captures do that. In
particular, the 4001-byte read/edit and 255-byte source-line captures take
precedence over a guessed interpretation of a published maximum.
04820a43474a63123be4b4c122cff97ec2a5fb1c
8f4736aff7c6c39d0c468985c4c5249b0137c39999b9b101b2882ee07ab1e36e  specs/file-formats/dbf.md
174cadbe2b2e7988b5dbae894cdc671038550751baa17334a80b4fec1ad2c000  specs/file-formats/dbt.md
921f8a2775e419a0b7416cccfa63d0192cc596dafb6360537045fb94862f7621  specs/file-formats/ndx.md
892ae4839b48ae4861f8416352498bc94f0b776f0f2695cf2f4cb18dc537903a  specs/file-formats/prg-and-aux-files.md
d8ac5689951bd302d6209ae2ac243ab96caad3c1ce42206ed465eb8b816b2135  specs/environment/work-areas-relations-and-limits.md
