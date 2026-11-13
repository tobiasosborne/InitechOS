# Kernel and desktop binding notes

The target archive has no kernel includes. Implement flair/include/flair_platform.h
in the platform integrator, not by adding kernel include paths to the Toolbox.
All calls are cooperative/task-context unless explicitly described otherwise.
This header is an internal host contract; apps still use only the SDK INT 81h ABI.

| Platform function | Legacy implementation / binding requirement |
|---|---|
| flair_platform_files_init | psp_build with zero psp_params_t, creating the kernel-owned shadow JFT, never the loaded image PSP |
| flair_platform_files_close | sft_close_process on that shadow PSP; safe repeated cleanup |
| flair_platform_files_live | Count shadow JFT entries 5..19 that are not JFT_CLOSED |
| flair_platform_file_info_get | sft_from_handle; accept only SFT_KIND_FILE, return open_mode, file_offset, dir_entry.file_size; no device records |
| flair_platform_path_is_device | int21_path_is_device, preserving DOS basename/device precedence |
| flair_platform_dos | int21_dispatch over the documented 68-byte frame; for a non-NULL file session save current PSP/CWD, install shadow PSP and root CWD, dispatch, restore even on error; NULL is system date/time context |
| flair_platform_swap_dta | int21_swap_dta; restore the prior DTA after the entire find-first/find-next loop |
| flair_platform_memory | int21_mcb_usage: total/free/largest in paragraphs, not bytes or KiB |
| flair_platform_load | loader_load_tenant; use supplied carve callback, return any carved block EVEN on subsequent failure, map tenant_plan fields explicitly; preserve serial loader status values (NOMEM=13) |
| flair_platform_tenant_call | tbx_tenant_call in the existing INT 81h assembly; cdecl fn/psp arguments and same saved-stack/crash return discipline |
| flair_platform_tenant_abort | tbx_tenant_abort; non-returning unwind of the active tenant call, not a normal C return |
| flair_platform_desktop_pattern | desktop_set_pattern, now policy delegated to the desktop host |
| flair_platform_speaker | cp_service's PIT channel 2: hz=0 clears port 61h bits 0/1; otherwise divisor=1193182/hz, control 43h=B6h, low/high divisor at 42h and port61 bits0/1 set; leave timing to tbx_preferences_tick |
| flair_platform_serial | existing serial_puts (ring-loss report consumed in task context) |
| flair_platform_panic | existing tbx panic halt, reached after PANIC tbx / HALTED serial markers; integrate the panic screen and halt, never return |

flair_platform_files is caller-owned 256-byte aligned opaque session storage.
A binding may put an opaque handle there if the new DOS context grows. The host
binding copies PSP/frame records rather than aliasing unrelated C types. DOS
file_info_get must reject invalid handles before accessing the JFT/SFT and return
only file handles belonging to this session. Handle modes are 0=read, 1=write,
2=read/write. File offsets and sizes are bytes. Pointer arguments in register
frames remain flat-32, as specified by the existing SDK; this is not a new ABI.

The platform load plan needs only psp_addr, load_base, module_len, entry and
block_len. Legacy loader_plan.params stays private to the loader. The carve
callback allocates from tbx_host.master via the existing flair_alloc path; the
Toolbox owns reverse-order failure reclamation. Do not silently free that block
inside the load binding as well. On load failure return the original diagnostic
number (the gate maps NOMEM to TBX_ERR_NOMEM, other failures to BADARG).

Time, memory and console inputs already have explicit APIs, so no extra implicit
kernel globals are needed: advance the Event Manager's flair_ticks clock at the
existing 100 Hz rate; tbx_bind supplies master arena, surface, process/window
lists, ROM cellfont, desk scrap and serial callback. Console/graphics hand-off
lives in the desktop pump (formerly kmain); no Toolbox routine owns video mode
entry/exit. Desktop integration must service tbx_take_affirm before tbx_reap,
repaint chrome before routed updates, and run modal/sf/file-copy requests only
after the tenant returns. The existing public API comments remain the authority.

There were no Toolbox COLD sections or executable overlay imports to remove.
The assembly INT 81h stub is parked in flair/desktop/tbx_gate.asm alongside
kmain_legacy.c and both imports' Finder/shell/film sources and tests; none is
built by the Toolbox Makefile. All linked C must agree on the ordinary ABI from mk/config.mk;
the old kernel's -mregparm=3/-mrtd image is not ABI compatible by assumption.
Rebuild the whole image and its platform binding with the default C ABI.

No direct kernel include remains in Toolbox C/private/public headers. The only
kernel includes introduced by the import are host integration tests:

flair/tests/test_tbx_file.c:9:#include "devices.h"
flair/tests/test_tbx_file.c:10:#include "int21.h"
flair/tests/test_tbx_file.c:11:#include "sft.h"
flair/tests/platform_dos.c:5:#include "int21.h"
flair/tests/platform_dos.c:6:#include "sft.h"
flair/tests/platform_dos.c:7:#include "find_data.h"

Those host-test includes are intentional debt, not target include-path exceptions.
The routing stub binding does not run file verbs; unused platform services abort.
The real DOS adapter retains caller PSP/CWD and SFT cleanup coverage in the file
suite. The target binding is deliberately not fabricated here: the kernel/desktop
engineers own the hardware and pump, and these are declarations for that work.

make -C flair now also runs make -C flair/region target, producing the ELF32
build/flair/region/target/region.o with the same TARGET_CC/TARGET_CFLAGS as the
Toolbox. Link that object alongside build/flair/libflair.a. The separate
build/flair/region/region.o remains a host test object, not a target input.
The minimal region target rule is marked for replacement by mk/target.mk when
conventions/hardening lands; this integration changes no mk/*.mk file.

All app records now come from sdk/include. tbx_layout.h compares menu manager
sizes and offsets to the SDK records and pins registration/event wire layouts
in target builds. cp_dialog_items.h is the private legacy dialog adapter.
The gate stores the DOS DTA as opaque bytes (SDK payload offset plus length);
the host real-DOS binding pins those values to kernel/include/find_data.h.
Kernel headers are confined to the host DOS test. Both borrowing scripts and
the old app GrafPort compatibility view have been removed. Apps use only SDK
headers. The loader's >64 KiB policy remains a vertical-slice integration task.
