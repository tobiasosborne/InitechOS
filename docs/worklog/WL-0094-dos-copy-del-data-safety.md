# DOS COPY/DEL data safety - audit K01-K04/K15

Context: branch lane-dos-dataloss, audit pass 5. Operator forbids bd and push;
issue ids in the commits identify the work without tracker writes.

## K01 - initech-vj28

COPY previously compared path text before destructive CREAT. COMMAND.COM now
opens the destination for a read-only identity probe and compares its resolved
parent-directory cluster and physical slot with the source handle. Distinct
empty files (both cluster zero) and identical names in different parents remain
distinct. Probe errors other than absence abort before CREAT.

Reference: Microsoft MS-DOS 3.3 User's Guide and User's Reference, July 1987,
User's Reference p. 50 (PDF p. 135): self refusal and zero-count footer. The
manual's general prohibition plus the resolved path model grounds alias handling;
no live real-DOS alias transcript was captured. Existing MSG-DOS-0020 is reused.

Acceptance: the new injected QEMU script first failed on SELF.BIN bytes changed
with SHELL-DONE and no triple fault. After repair mtools extracted all 8192 and
80928 bytes intact, with root/absolute/drive/dot/parent/CWD aliases refused;
the 114-byte control is intact too. fsck.fat -n is clean. Host integration checks
resolved identity including separate empty entries, with an unchanged disk.
The false-identity host mutant fails resolved entry identity. The existing
paired no-self-guard/no-read-CF shell mutant changes SELF.BIN, so the independent
byte oracle goes RED. Existing tests were not weakened.

Friction: this sandbox denies Unix bind(2); the unchanged test-shell failed
before boot. INITECH_QMP_STDIO=1 selects QEMU's documented -qmp stdio over an
inherited socketpair, using exactly the same key injection and grading. DOS
path/pattern keys add semicolon/slash QKeyCodes. The source default remains
Unix QMP; the environment mode uses no listener or shared port.

Before changes: test-kernel-headroom green, flagship 183848 loaded / 12760 free;
largest existing mutant 184552 / 12056 free (107 kernels).

Evidence: build/dosfix-evidence/ logs and build/dos_safety_*/ stopped images,
commands, serial, mcopy extraction and fsck output (build intermediates).
