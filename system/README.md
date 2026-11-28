# Product composition

Links the InitechDOS objects and resident COMMAND.COM with the FLAIR Toolbox,
region engine and desktop binding into build/system/INITDOS.SYS. The standalone
kernel artifact remains build/kernel/INITDOS.SYS. Dependencies: kernel, dos,
flair and flair/desktop; InitechCalc supplies the frame's data-volume inputs.
No kernel source is compiled here and no ABI flags are changed.

S1 verification: the QEMU and Bochs DOS smoke passes (8 MiB); the composed
image is 186010 loaded bytes. A working disk with AUTOEXEC.BAT containing
FLAIR followed by ECHO hook-complete reports FLAIR is not installed and
reaches hook-complete. This is the weak-hook milestone, not a desktop claim.

S2: factory/smoke-desktop.sh now checks Finder startup, pointer movement,
Special > Exit to InitechDOS (including its confirmation), a real A:\>
prompt, and a second FLAIR launch. QEMU reports zero faults and two power-on
CPU resets. Screens: build/smoke-desktop/{desktop,dos,qemu}.ppm.

S3 working-volume.list supplies the APPS directory from imported application
artifacts. Product AUTOEXEC.BAT ends with FLAIR. The DOS smoke deletes that
file from its private writable volume copies, retaining the product kernel.
Finder opens INITECH then APPS; the APPS view contains 123.EXE and 123.MNU.
The QEMU folder screenshot is build/smoke-s3/apps.ppm.

S4: factory/smoke-desktop.sh drives 123.EXE through the Finder, enters 123 and
1.5 and @SUM(A1..B1), saves SLICE.WK1 through /File Save, quits, exits FLAIR,
and starts it again. mtools extracts the file and the independent Lotus corpus
reader verifies all three cells, formula RPN and cache 124.5. Captures include
sum.ppm. The kernel file cap is removed with overflow guards on PSP + rounded
file + BSS; the supplied heap carve retains the normal NOMEM result.

Final interface: kernel/ has a weak environment entry and a public console
restore function; dos/command/ calls it for FLAIR. The desktop strong entry
returns to that caller. On-disk apps use only INT81 through sdk/. Kernel input,
loader and framebuffer types used by the binding are public in kernel/include/.

make test builds both links, checks a strong environment entry in the product
and a weak one with no GUI symbols in the standalone DOS image, verifies archive
inputs, and runs the factory transport parser check. Product sources use no
overlay, section GC, regparm, callee-pop ABI or LTO.

The data-volume inputs have a separate three-column source/destination/attribute
contract in data-volume.list. Run `sh system/install-data.sh IMAGE` from the
source root on a fresh formatted ATA A: volume; the factory then fixes its FAT
dates. It installs the desktop owner's frame arrangement as hidden DESKTOP.DB
and the two InitechCalc frame inputs in the root. No system programs are added
by this data manifest. `tests/data-volume.sh` uses mattrib, normal/hidden mdir
listings and mcopy to grade the attributes and owner-supplied bytes independently.
This prepares the C: delivery layout; the factory activates the new data manifest
when the complete working system moves to C:.

Installed inputs are an explicit two-column source/destination contract in
working-volume.list, including COMMAND.COM and the closed utility list.
`make installed-inputs` builds the Initech-authored DBF examples with a small
C factory producer, converts the introductory Pascal and accounting program
to CRLF, copies recovered hash-pinned TPS inputs, and installs the complete
matching Turbo source. Dependencies for these inputs also include Turbo
Initech (source and compiler), and InitechBase (sample/canon references).
The image composition owner must build `turbo` before consuming the manifest.
Missing sources fail; no build reads legacy or attic. Ownership, attributes,
producers, source captures and imported hashes are in samples/PROVENANCE.md.
All existing APPS artifacts remain owned/built by their named components.
The APPS directory retains its original fourteen entries. InitechBase and TPS
examples live in the root SAMPLES directory; the compiler and its Pascal
sources live in the root TURBO directory. Root README.TXT gives the commands.
Separate application/example folders follow the existing APPS convention and
keep every original application visible in the default Finder window. The
host layout check compares APPS against its pre-example inventory; desktop
smoke exercises the resulting icons on both emulators.

`make installed-inputs` also writes `CONFIG.SYS` and `AUTOEXEC.BAT` under
`build/system/installed/c-boot/` for the staged C: system disk. The approved
GM8 delivery design sets C: PATH, COMSPEC and PROMPT and leaves the user at
the DOS prompt to type FLAIR. The working-volume manifest still selects the A:
files directly in `installed/`. The staged builder uses the C: files.
The separate `c-boot/` path expires at activation step 20.

Both working systems carry `ANSI.SYS` alongside `CONFIG.SYS`. The file
selects the resident ANSI console interpreter under the DEVICE contract
in `kernel/README.md`. Its contents explain that role to a user at DOS.

`make test` additionally checks the required workflow inventory, independently
reads the three DBFs in Python, verifies source/protocol/canon input identity,
and compares both sets of boot files byte for byte. `make mutants` (alias
`manifest-mutants`) uses isolated patches to check inventory, data-volume
attributes, C: startup bytes and capacity selection. Python only reads the
foreign DBF format; C and shell produce all shipped bytes. Generated tables
have a fixed 1993-07-19 update date. Recovered
TPS bytes retain the original table date. These examples have no memo/index
dependency, so no DBT/NDX is installed.

`make smoke` builds the product images and drives the staged C: system on
QEMU and Bochs at 8 MiB. The test checks the active FAT12 partition, the
hidden boot entries and their bytes, the visible files against their producers,
and the available working space before boot. Each run uses disposable disk
copies with the shipped CONFIG and AUTOEXEC. The first command runs at the C:
prompt and reports the inherited PATH and COMSPEC.

The TPS run keeps DATA attached and checks its disk stays byte-identical.
The report comes from the written C: partition, including the Bochs private
master. It matches the retained REALDOS report with the four installed
tables in its census and the expected FCB padding. The Base run has no ATA
slave. It starts INITBASE from APPS beside its overlay, checks LIST and
accounting output against the retained III+ and canon references, and reads
back a short file written after QUIT to verify return to the C: shell.

The Turbo Initech run also boots C: alone. It opens the IDE by typing TURBO,
loads the pinned DEMO.PAS and checks Error 85 on the real DOS console page.
It returns to DOS, compiles HELLO.PAS by name, runs TURBOOUT.COM and
compares stdout with Free Pascal. The check reads the executable from the
written C: disk and compares the installed compiler source with the current
Turbo source.

Evidence remains in `build/system/installed-smoke.*`. For focused runs, set
`SYSTEM_SMOKE_CPUS` to one emulator or `SYSTEM_SMOKE_WORKFLOWS` to a subset of
`tps base turbo`. The default runs all workflows on both emulators.

`make capacity` builds the image set and measures the installed C: partition
with mtools. It requires 2 MiB free for the compiler's listing, two output
generations, the full source input copy, reports and save stages. The check
uses the staged `c.img` and `a-data.img`. With a boot-only master it grades
the legacy A: volume; use `SYSTEM_BOOT_IMAGE` and `SYSTEM_DATA_IMAGE` to
select another pair or `SYSTEM_WORK_IMAGE` to grade a volume explicitly.
The host fixture checks a roomy C: beside a 1.44 MB A:, and requires refusal
when A: is the working volume. A hosted run of the exact shipped source
produced a 954720-byte listing and 164263-byte native executable, before the
source copy and retained second generation. Capacity is separate from the
ordinary Hello workflow so a passing short program cannot certify the finale.

The boot/working disk split is unchanged here. IO.SYS and composed INITDOS.SYS
remain the boot-volume builder's responsibility. Consolidating to C:, expanding
the working volume, removing the builder's DOS wildcard, and scheduling Turbo
before image assembly belong to the image composition owner.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: startup
Interface use-binding: kernel/include/blockdev.h
Interface use-binding: kernel/include/conedit.h
Interface use-binding: kernel/include/devices.h
Interface use-binding: kernel/include/diskutil.h
Interface use-binding: kernel/include/dos_structs.h
Interface use-binding: kernel/include/environment.h
Interface use-binding: kernel/include/fat12.h
Interface use-binding: kernel/include/fileio_fat.h
Interface use-binding: kernel/include/find_data.h
Interface use-binding: kernel/include/idt.h
Interface use-binding: kernel/include/int21.h
Interface use-binding: kernel/include/io.h
Interface use-binding: kernel/include/memory_map.h
Interface use-binding: kernel/include/power.h
Interface use-pure: kernel/include/mcb.h
Interface use-records: kernel/include/drive_letters.h
<!-- end-interface-declarations -->
