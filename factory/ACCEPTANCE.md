# Spine acceptance record

Local verification on 2026-11-13, spine/boot-kernel. These are measurements,
not build inputs. Complete run outputs are retained under ignored build/:
smoke/ is the final product run, evidence/ preserves parity, red, oversized,
low-memory and clean-build/repro results across clean operations.

A. `make clean && make image`: succeeds in 7.60 seconds wall time on this host.
INITDOS.SYS is 105848 loaded bytes, no sector padding (207 FAT clusters).
The image includes the DOS-only kernel and fifteen external .COM utilities.

B. `make smoke`:

```
ok QEMU: banner, VER, DIR, directories, TYPE, COPY, REN, DEL, IF/GOTO, SORT, MORE, FIND, CHKDSK
ok mtools: written bytes, SORT order, MORE passthrough, REN/DEL and RD persisted
ok QEMU: no triple fault, guest error or unexpected CPU reset
ok Bochs: VGA fallback, banner, prompt, VER and DIR
SMOKE: 4 checks, 0 failures (QEMU + Bochs, 8 MiB)
```

Key actual serial lines (CR stripped here):

```
InitechDOS  Version 3.30
A:\>ver
InitechDOS Version 3.30
 Directory of A:\
A:\CHECK>type proof.txt
written-by-command
        1 File(s) copied
batch-ok
apple
zebra
  1457664 bytes total disk space
A:\>echo smoke-complete
smoke-complete
```

QEMU report: `triple_fault=0 cpu_reset=2 guest_errors=0`, `OK=1`.
Bochs report: `triple_fault=0`, `OK=1`; its loader emits VBE-E00 then VGA13.
The guest file read back by mtools is exactly `written-by-command\r\n`;
SORTED.TXT is `apple\r\nzebra\r\n`; MORE.TXT equals WORDS.TXT. Both the deleted
renamed copy and removed EMPTY directory are absent to mtools.

C. Replaced the builtin VER prefix with `InitechDOS Wrong Version`, ran
`make smoke`, and observed nonzero exit with:
`FAIL QEMU VER: missing line [InitechDOS Version 3.30]`.
Restored the source, rebuilt, then ran the final green smoke above. See
build/evidence/red.log; no wrong code or new mutation macro remains in source.
The test did not accept a mere keyboard echo of the completion command.

D. `grep -rn 'include.*flair\|include.*apps/' kernel dos`: no matches.
`grep -c 'BOOT_' kernel/main.c`: 0. `wc -l kernel/main.c`: 14.
The remaining startup wiring is in startup.c, not another boot personality.

E. `make repro`: two clean builds compare equal, including kernel bytes:

```
f7acafbcc9124a25c6c1a26f3053c8059c098d4671740e5aea555c7194e4f9b9  build/image/boot.img
ad8ce9fa0e492f282bee880c63d385ac0d06ce3e28ec79f8ceb34805ae2eba7d  build/image/data.img
19f2607679594684bbc2af47dcb3e890fc2d532fa66dcbb1e9de726ecb2800cc  build/kernel/INITDOS.SYS
REPRO: identical across two clean builds
```

The final clean production build also passes these same hash checks.

F. `make -C boot bigkernel-check` succeeds end to end, restores the normal
image, and prints on each emulator:

```
BIGKERNEL-OK size=237112 sum=00FF0000
BIGKERNEL: 2 checks, 0 failures
```

It checks all 131072 blob bytes after BSS/runway clearing, beyond the retired
196608-byte limit. A separate 7 MiB QEMU run emits
`Insufficient extended memory: 8 MB required`, with no KERNEL marker and OK=1.
Completion-only QMP handling in the imported factory driver was corrected so
both checks quit on their actual markers instead of timing out.

G. `sh factory/parity.sh` boots the absolute read-only legacy tracer_boot.img
and the product through the same factory/session.txt key script, using copies
of the same pristine data volume. The complete serial transcript starting at
SHELL-READY is byte-identical, including date/time prompts, DIR, file commands,
batch, filters and CHKDSK. No command-output differences are normalized away;
only CR removal is applied equally to both before comparison.

Every pre-prompt raw serial difference is accounted for:

- INITDOS-CLUSTERS: 0180 becomes 00CF because the old padded 384-sector image
  becomes 105848 loaded bytes / 207 clusters.
- VIDEO, KAT, BI-OK, EXTMEM, CONSOLE and FLAIR-HEAP-OK diagnostics are absent
  from the smaller production entry. Stage2 still checks the K386 header,
  physical bounds and minimum extended memory; console initialization still
  succeeds or stops explicitly. There is no FLAIR heap initialization here.
- IDT-SELFTEST-OK and IDT-RESUMED are gone with the boot-time INT80 test and
  its unused ISR. The real interrupt/IRQ/syscall paths remain exercised by
  both emulators' interactive sessions.

H. Files were claimed with git mv; retired overlay/transient linker/header
files were deleted. LC_ALL=C non-ASCII scan of boot/kernel/dos/factory and
mk/target.mk has no matches. git diff --check is clean. Logical commits are
on spine/boot-kernel only, no push. Final git status is checked after the
acceptance documentation commit.

Limitations are explicit, not waived by these checks: kernel and DOS tier 0
are still pending; COMPONENTS is preserved, with no false completed imports.
The resident shell/internal API coupling, utility INT13/25/26/MCB/8042 access,
fixed format/loader limits and legacy deferred CONFIG.SYS behavior are listed
in kernel/README.md and dos/README.md. This does not yet meet a strict
INT21-only boundary for every utility. Tests retain old codenames/paths
untouched; production sources have none. The exact remaining disabled mutation
inventory is kernel/tests/MUTANTS-TODO.md. The installed Bochs model is pentium,
matching the legacy driver; its build offers no 486 model. QEMU is CPU 486,
8 MiB, with ACPI off.
