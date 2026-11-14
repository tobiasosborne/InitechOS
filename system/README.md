# Product composition

Links the InitechDOS objects and resident COMMAND.COM with the FLAIR Toolbox,
region engine and desktop binding into build/system/INITDOS.SYS. The standalone
kernel artifact remains build/kernel/INITDOS.SYS. Dependencies: kernel, dos,
flair and flair/desktop. No kernel source is compiled here and no ABI flags are changed.

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
inputs, and runs the factory transport parser check. make smoke delegates to
the complete product session. Product sources use no overlay, section GC,
regparm, callee-pop ABI or LTO. make mutants fails with its actual pending work.

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

`make test` additionally checks the required workflow inventory, independently
reads the three DBFs in Python, and verifies source/protocol/canon input
identity. `make manifest-mutants` rejects an omitted table and an installed
test driver using isolated manifest patches. Python only reads the foreign DBF format; C and shell produce all
shipped bytes. Generated tables have a fixed 1993-07-19 update date. Recovered
TPS bytes retain the original table date. These examples have no memo/index
dependency, so no DBT/NDX is installed.

`make smoke` builds the ordinary product image and drives the installed TPS,
Base and Turbo workflows on QEMU and Bochs at 8 MiB. Each session clones both
product disks, removes AUTOEXEC on its private working copy to reach DOS,
sets the ordinary root PATH, and changes to SAMPLES or TURBO. The Base session
starts in APPS beside its required overlay and opens the examples using
absolute SAMPLES paths. No fixture or executable is
injected. Every manifest file is independently extracted and compared before
boot. TPSREPT.TXT matches all meaningful bytes of the retained REALDOS report,
with its directory census changed explicitly from the original single TPS.DBF
to the four installed DBFs. The comparator also checks FCB record padding and
rejects any other difference. Base LIST
rows come from real III+ captures; the accounting report retains the canon
negative ages and 0.00 total. Turbo compiles the installed HELLO via its
current fixed-name protocol, runs the result, and agrees with FPC stdout.
Artifacts remain in build/system/installed-smoke.*. SYSTEM_SMOKE_CPUS can
select a single emulator for iteration; the default runs both.

`make capacity` independently measures free clusters with mtools and requires
2 MiB free for the compiler's listing, two output generations, the full source
input copy, reports and save stages. This fails on the inherited 1.44 MB volume;
the image owner must enlarge it. A hosted run of the exact shipped source
produced a 954720-byte listing and 164263-byte native executable, before the
source copy and retained second generation. Capacity is separate from the
ordinary Hello workflow so a passing short program cannot certify the finale.

The boot/working disk split is unchanged here. IO.SYS and composed INITDOS.SYS
remain the boot-volume builder's responsibility. Consolidating to C:, expanding
the working volume, removing the builder's DOS wildcard, and scheduling Turbo
before image assembly belong to the image composition owner. The installed
README.TXT describes the currently available fixed-name compiler protocol;
when the Turbo owner lands the IDE, update those directions with its workflow.

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
