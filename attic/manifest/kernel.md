# kernel

The file inventory includes kernel-owned test guest fixtures; their location does not make them shipping kernel code. `kmain.c` is listed once here for accounting, with the desktop-owned slices mapped below and in flair-desktop. MILTON currently links FLAIR even in the shell kernel; no standalone GUI-free kernel artifact exists at the tag.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

97 production/source-support files; 41,123 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/milton/absdisk_program.asm:1`|246|the baked flat (.COM-equivalent) INT 25h / INT 26h **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/ansi.c:1`|640|InitechDOS ANSI.SYS escape-sequence interpreter: pure FSM.|
|`os/milton/ansi.h:1`|207|InitechDOS ANSI.SYS escape-sequence interpreter: pure FSM.|
|`os/milton/ansi_cursor.h:1`|26|DOS 3.30 TR Ch 4 CUD/CUF/CUU/CUB bridge. Shared by the live kernel|
|`os/milton/ansi_probe_program.asm:1`|106|ANSI console regression probe, test fixture only (not on the shipped volume). **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/ata.c:1`|620|- minimal ATA PIO LBA28 sector read (MILTON).|
|`os/milton/ata.h:1`|159|- minimal ATA PIO LBA28 sector read backend (MILTON).|
|`os/milton/baked_exec_probe.asm:1`|34|Factory-only baked EXEC probe: resident caller must survive nesting. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/block_device.c:1`|184|Physical INT13, DEC-17. IBM BIOS TR pp2-58..67; IBM DOS TR pp9-8..9.|
|`os/milton/block_device.h:1`|19|ADR-0003 DEC-17. IBM DOS 3.30 TR pp9-6..9; BIOS TR pp2-58..67.|
|`os/milton/block_device_probe.asm:1`|75|Oracle authored before service: IBM DOS TR pp9-7..9, DEC-17. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/blockdev.h:1`|53|- sector-granular block-device interface (MILTON).|
|`os/milton/boot_info.h:1`|74|stage2 -> C kernel parameter block.|
|`os/milton/boot_screen.h:1`|203|the FLAIR boot screen (bead initech-tdnl.105): the Mac OS|
|`os/milton/conedit.h:1`|106|Shared DOS template editor. Ref: ADR-0003 Appendix A; MS-DOS 3.3 User's|
|`os/milton/config_sys.c:1`|300|PURE CONFIG.SYS parser (beads initech-509.2).|
|`os/milton/config_sys.h:1`|87|PURE CONFIG.SYS parser for InitechDOS SYSINIT (beads initech-509.2).|
|`os/milton/conin_program.asm:1`|86|the baked flat (.COM-equivalent) CON-INPUT test program. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/conkeys_program.asm:1`|49|Test client, flat .COM-equivalent (ADR-0003 DEC-08 / Appendix A). **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/console.c:1`|630|InitechDOS 80x25 LFB text console implementation. **MIXED platform logic + GUI visual rendering; retain platform behavior, move presentation**|
|`os/milton/console.h:1`|193|InitechDOS 80x25 LFB text console (the VGA 8x16 ROM font path).|
|`os/milton/datetime_program.asm:1`|255|the baked flat (.COM-equivalent) DATE/TIME program. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/devices.c:1`|710|InitechDOS heritage DOS character-device chain: resident|
|`os/milton/devices.h:1`|391|InitechDOS heritage DOS character-device chain: device-header|
|`os/milton/dir_program.asm:1`|83|the baked flat (.COM-equivalent) DIR program. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/dospath.h:1`|90|Drive-qualified DOS path text, shared by the external commands|
|`os/milton/exec_high_fixture.asm:1`|142|Independent DEC-03a.2-.5 oracle. Literal header, addresses and ABI; no **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/exec_high_low.asm:1`|12|DEC-03a conventional nested child; PIC, remains on its low MCB stack.|
|`os/milton/execguard_program.asm:1`|128|EXECGD.COM: a resident flat program that EXECs the **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/exith_program.asm:1`|83|the FAT-sourced "leaky" child for the EXIT-handle oracle. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/fat12.c:1`|4968|- FAT12 volume mount + BPB parse (MILTON).|
|`os/milton/fat12.h:1`|1255|- FAT12 volume mount + read interface (MILTON).|
|`os/milton/fcbx_program.asm:1`|592|FCBX.COM, the FCB legacy-API exerciser (beads initech-509.9). **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/fdisk_fixture.asm:1`|35|Factory-only flat probe. DEC-17: install a scratch MBR AFTER boot, allowing **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/fileio_fat.c:1`|1137|FAT12-backed INT 21h file backend (beads initech-0qh; epic|
|`os/milton/fileio_fat.h:1`|59|FAT12-backed INT 21h file backend (beads initech-509.5).|
|`os/milton/fpu.h:1`|91|x87 FPU bring-up: the PURE decision logic (beads initech-zj6w).|
|`os/milton/gobble_program.asm:1`|88|the FAT-sourced flat (.COM-equivalent) GOBBLE program. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/greet_program.asm:1`|56|the FAT-sourced flat (.COM-equivalent) GREET program. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/hygiene_probe.asm:1`|51|Factory-only flat probe. ADR-0003 Appendix A 3Ch/3Eh/56h; **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/idt.c:1`|129|IDT table + gate encode + lidt. See idt.h for the full citations.|
|`os/milton/idt.h:1`|166|InitechDOS Interrupt Descriptor Table (32-bit protected/flat).|
|`os/milton/int21.c:1`|6311|InitechDOS INT 21h dispatcher (the `int 0x21` syscall spine).|
|`os/milton/int21.h:1`|1054|InitechDOS INT 21h dispatcher (the `int 0x21` syscall spine).|
|`os/milton/io.h:1`|25|freestanding x86 port I/O primitives.|
|`os/milton/irq.c:1`|90|in-IRQ depth counter + the INT 21h reentrancy fail-loud panic.|
|`os/milton/irq.h:1`|71|in-IRQ depth counter + the INT 21h reentrancy guard hook.|
|`os/milton/irqstorm_program.asm:1`|303|the baked flat (.COM-equivalent) IRQ-STORM program. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/isr.asm:1`|639|CPU exception entry stubs + common dispatch trampoline.|
|`os/milton/kbd.c:1`|475|PS/2 keyboard driver: ring buffer + scancode set 1 -> US ASCII +|
|`os/milton/kbd.h:1`|162|PS/2 keyboard (IRQ1) driver: scancode set 1 -> US ASCII + ring.|
|`os/milton/kbd_echo_program.asm:1`|62|initech-wuxo0: real program reads alternating INT 21h AH=07h/08h bytes. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/kernel.ld:1`|71|flat binary link script for the InitechDOS C kernel.|
|`os/milton/kmain.c:1`|8207|InitechDOS flat C kernel entry (the stage2 handoff landing). **MIXED kernel + desktop + compiled-in tests; line map below**|
|`os/milton/kstart.asm:1`|113|InitechDOS flat C kernel entry stub (32-bit protected/flat).|
|`os/milton/loader.c:1`|1480|InitechDOS flat program loader (lay out PSP + image, run, return).|
|`os/milton/loader.h:1`|492|InitechDOS flat program loader (lay out PSP + image, run, return).|
|`os/milton/loader_cold.c:1`|566|DEC-03a.7, initech-jarzu: cold loader validation and refusal paths. **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`os/milton/loader_cold.h:1`|13|DEC-03a.7, initech-jarzu; ADR-0014 generated COLD export/import seam. */ **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`os/milton/mcb.c:1`|439|InitechDOS memory arena: the MCB chain behind AH=48h/49h/4Ah.|
|`os/milton/mcb.h:1`|146|InitechDOS memory arena (the MCB chain behind AH=48h/49h/4Ah).|
|`os/milton/mouse.c:1`|256|PS/2 aux mouse (IRQ12) driver. See mouse.h for the full citations.|
|`os/milton/mouse.h:1`|71|PS/2 aux mouse (IRQ12) driver: the FLAIR live-loop pointer source.|
|`os/milton/mouse_pack.h:1`|121|pure PS/2-delta -> FLAIR_RAW_MOUSE payload packing.|
|`os/milton/multiopen_program.asm:1`|244|the baked flat (.COM-equivalent) MULTI-OPEN program. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/mz.c:1`|281|InitechMZ header-parse + flat-relocation-apply (pure logic unit).|
|`os/milton/mz.h:1`|140|InitechMZ header-parse + flat-relocation-apply API.|
|`os/milton/mzexec_fixture.asm:1`|121|the InitechMZ (.EXE) RUNTIME-PROOF load module. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/native.c:1`|107|ADR-0009 DEC-03a.4, initech-jarzu: native first-fit blocks inside the high|
|`os/milton/nested_exec_program.asm:1`|163|Factory fixture: resident flat caller, real AH=4Bh/4Dh and INT20 return. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/overlay.ld:1`|58|link script for C:\FLAIR.OVL (ADR-0014 DEC-14-1/-2/-3; epic|
|`os/milton/overlay_hdr.asm:1`|46|the header of C:\FLAIR.OVL (ADR-0014 DEC-14-2; epic|
|`os/milton/ovl_fixture.asm:1`|34|the InitechMZ OVERLAY load module for make test-exec-overlay. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/ovltest_program.asm:1`|233|OVLTEST.COM, the in-emulator driver of INT 21h **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/panic.c:1`|239|fail-loud CPU-exception dispatcher (the IDT common-stub C side). **MIXED platform logic + GUI visual rendering; retain platform behavior, move presentation**|
|`os/milton/panic.h:1`|11|Terminal non-CPU diagnostics: physical display, no heap or console state.|
|`os/milton/panic_program.asm:1`|44|Test-only FAT-loaded .COM, ADR-0003 DEC-08 / Appendix A (3Ch/40h/3Eh). **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/panic_screen.h:1`|115|initech-s25. PRD Sec 5 (VGA 8x16), Appendix B (printer-error pastiche).|
|`os/milton/partition_probe.asm:1`|68|Factory-only flat test program, never installed on the shipped volume. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/pic.c:1`|90|8259A remap + mask-all. See pic.h for the full citation set.|
|`os/milton/pic.h:1`|47|8259A Programmable Interrupt Controller remap + mask.|
|`os/milton/pit.c:1`|124|Intel 8254 PIT channel 0 tick. See pit.h for citations.|
|`os/milton/pit.h:1`|72|Intel 8254 Programmable Interval Timer (channel 0, IRQ0) tick.|
|`os/milton/psp.c:1`|406|InitechDOS Program Segment Prefix (PSP) construction.|
|`os/milton/psp.h:1`|151|InitechDOS Program Segment Prefix (PSP) construction.|
|`os/milton/rec_sync.h:1`|237|- included once, by os/milton/kmain.c (it uses kmain's|
|`os/milton/rtc.c:1`|314|InitechDOS MC146818 RTC / CMOS clock source (beads initech-yv9).|
|`os/milton/rtc.h:1`|104|InitechDOS MC146818 RTC / CMOS clock source (beads initech-yv9;|
|`os/milton/sft.c:1`|382|InitechDOS System File Table (SFT) + Job File Table (JFT) layer.|
|`os/milton/sft.h:1`|240|InitechDOS System File Table (SFT) + Job File Table (JFT) indirection.|
|`os/milton/sysinit.c:1`|533|InitechDOS SYSINIT phases (beads initech-509.2).|
|`os/milton/sysinit.h:1`|82|InitechDOS SYSINIT: the named, ordered system bring-up contract|
|`os/milton/sysinit_program.asm:1`|90|the FAT-sourced child that proves the FILES= cap BITES. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/tsr_program.asm:1`|157|the test-tsr fixture programs (bead initech-6fdt). **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/type_program.asm:1`|87|the baked flat (.COM-equivalent) TYPE program. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/vect_program.asm:1`|183|the baked flat (.COM-equivalent) SETVECT/GETVECT + INT 24h **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/write_program.asm:1`|135|the baked flat (.COM-equivalent) WRITE round-trip program. **test guest or external entry; inspect deployment before treating as product**|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`os/milton/.gitkeep`|binary|.gitkeep; source/contract input (role inferred from path)|
|`os/milton/test_40oq.c:1`|659|CAPSTONE: INT 21h Appendix-A coverage certificate (beads initech-40oq).|
|`os/milton/test_ansi.c:1`|1084|host unit oracle for the ANSI.SYS escape-sequence FSM|
|`os/milton/test_ansi_bridge.c:1`|29|initech-fzsvo: exercise the SAME adapter as kmain with the real console.|
|`os/milton/test_ansi_wire.c:1`|439|host oracle for WIRING the ANSI.SYS FSM into the CON|
|`os/milton/test_arena_disjoint.c:1`|389|host oracle for the AH=48h MCB arena being PROVABLY|
|`os/milton/test_ata.c:1`|99|Scripted ATA bus oracle. Ref: T13/1321D rev3 Sec 7.4 Command register,|
|`os/milton/test_config_sys.c:1`|198|host unit oracle for the CONFIG.SYS parser (beads|
|`os/milton/test_config_sys_fuzz.c:1`|524|deterministic, seeded generative fuzzer for the PURE|
|`os/milton/test_conin.c:1`|855|host unit oracle for the INT 21h CON INPUT functions|
|`os/milton/test_console.c:1`|604|- host blit-math oracle for the LFB text console.|
|`os/milton/test_devices.c:1`|814|host unit oracle for the DOS character-device chain|
|`os/milton/test_devwire.c:1`|507|host unit oracle for the INT 21h <-> character-device-chain|
|`os/milton/test_dos_safety.c:1`|136|Host integration of the real INT21/FAT stack. The thin shell grades disk|
|`os/milton/test_exec.c:1`|537|host unit oracle for INT 21h AH=4Bh EXEC + AH=4Dh GET-RETURN-|
|`os/milton/test_fileio.c:1`|2437|host unit oracle for the INT 21h file-handle functions|
|`os/milton/test_fileio_subdir.c:1`|941|- INT 21h subdir OPEN/READ over the REAL FAT12|
|`os/milton/test_fpu.c:1`|102|- host oracle for the x87 bring-up DECISION logic|
|`os/milton/test_idt.c:1`|133|host unit oracle for the IDT gate encode + interrupt-frame|
|`os/milton/test_int21.c:1`|1234|host unit oracle for the INT 21h dispatch logic|
|`os/milton/test_int21_edge.c:1`|737|host unit oracle HARDENING the INT 21h error paths and|
|`os/milton/test_int21_wildcard.c:1`|180|host unit oracle for the DOS 8.3 FCB-style wildcard|
|`os/milton/test_int24.c:1`|386|host unit oracle for the INT 22/23/24 handlers, SETVECT/|
|`os/milton/test_int24_wired.c:1`|339|host oracle for the INT 24h critical-error WIRING|
|`os/milton/test_kbd.c:1`|480|host unit oracle for the PURE keyboard/PIT logic (beads|
|`os/milton/test_keep.c:1`|382|host oracle for INT 21h AH=31h KEEP (Terminate and Stay|
|`os/milton/test_loader.c:1`|271|host unit oracle for the flat program loader's PREP|
|`os/milton/test_loader_big.c:1`|140|host unit oracle for the FAT IN-PLACE loader prep|
|`os/milton/test_low_mapping.h:1`|27|Factory only. Rule 11; 4aql/ahp2/ugsk: fake DOS segments must not depend on|
|`os/milton/test_mcb.c:1`|323|host property suite for the MCB memory arena (initech-509.6).|
|`os/milton/test_mcb_int21.c:1`|274|host dispatch oracle for the INT 21h memory functions|
|`os/milton/test_mz.c:1`|715|host unit oracle for the InitechMZ header-parse +|
|`os/milton/test_mzload.c:1`|481|host unit oracle for the InitechMZ (.EXE) loader integration|
|`os/milton/test_native.c:1`|44|Independent DEC-03a.4 boundary/property vectors; byte snapshots on refusals. */|
|`os/milton/test_overlay.c:1`|734|host oracle for INT 21h AH=4Bh AL=03h LOAD OVERLAY.|
|`os/milton/test_prog.h:1`|88|the baked flat test program image (beads initech-509.5).|
|`os/milton/test_program.asm:1`|58|the baked flat (.COM-equivalent) test program.|
|`os/milton/test_psp.c:1`|315|host unit oracle for PSP construction (beads initech-509.4).|
|`os/milton/test_redir.c:1`|337|host unit oracle for STDOUT-handle redirection of the|
|`os/milton/test_rtc.c:1`|182|host unit oracle for the PURE RTC conversion logic (beads|
|`os/milton/test_sft.c:1`|306|host unit oracle for the SFT/JFT handle layer (beads|
|`os/milton/test_tenant_load.c:1`|238|host oracle for the DEC-08a PARAMETERIZED-BASE|
|`os/milton/test_tsr.c:1`|235|host oracle for the InitechDOS TSR kernel mechanics|
|`os/milton/test_tsr_io.c:1`|135|host oracle for the two device halves of the TSR contract|
|`spec/absdisk_int2526.json:1`|67|those are AH-selected INT 21h functions already listed in spec/int21h_register.json and are entirely distinct from these vectors. Ratified by ADR-0003 Amendment DEC-15 (OEA-ADR-0003-A3). Thi|
|`spec/block_device_contract.h:1`|20|Generated by tools/gen_block_device_spec.c; ADR-0003 DEC-17. */|
|`spec/block_device_contract.json:1`|27|"schema_version": 1,|
|`spec/chimera_element_map.json:1`|148|"_comment": [|
|`spec/dos_config_sys_baseline.txt:1`|7|DEVICE=ANSI.SYS|
|`spec/dos_structs.h:1`|303|- InitechDOS on-disk and in-memory structure layouts.|
|`spec/drive_letters.h:1`|15|Generated by tools/gen_drive_letters.c; ADR-0003 DEC-18. */|
|`spec/drive_letters.json:1`|20|"schema_version": 1,|
|`spec/find_data.h:1`|90|- InitechDOS FINDFIRST/FINDNEXT Disk Transfer Area layout.|
|`spec/fpu_contract.h:1`|41|- the locked x87 boot control word (beads initech-zj6w).|
|`spec/hardware.json:1`|152|"_comment": [|
|`spec/initechmz_extended.h:1`|25|LOCKED: ADR-0009 Amendment DEC-03a (RATIFIED 2026-11-06), initech-ayj36.|
|`spec/int21h_calling_convention.json:1`|170|"_comment": [|
|`spec/int21h_register.json:1`|56|"_comment": [|
|`spec/memory_map.h:1`|720|- InitechDOS flat (32-bit) physical memory map constants.|
|`spec/overlay_format.h:1`|137|- the LOCKED FLAIR.OVL container (Rule 8).|
|`spec/tsr_contract.h:1`|104|- the InitechDOS TERMINATE-AND-STAY-RESIDENT contract|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`os/milton/block_device.c:4`|`block_device_contract.h` → `spec/block_device_contract.h`|PUBLIC INTERFACE|
|`os/milton/block_device.c:5`|`drive_letters.h` → `spec/drive_letters.h`|PUBLIC INTERFACE|
|`os/milton/block_device.h:7`|`diskutil.h` → `os/milton/diskutil.h`|INTERNAL LEAK|
|`os/milton/boot_screen.h:34`|`boot_logo.h` → `spec/assets/boot_logo.h`|PUBLIC INTERFACE|
|`os/milton/boot_screen.h:42`|`asset_lz.h` → `os/flair/asset_lz.h`|INTERNAL LEAK|
|`os/milton/boot_screen.h:43`|`asset_lz_data.h` → `os/flair/asset_lz_data.h`|INTERNAL LEAK|
|`os/milton/boot_screen.h:46`|`text.h` → `os/flair/text.h`|INTERNAL LEAK|
|`os/milton/console.c:32`|`../flair/surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/milton/fat12.h:25`|`dos_structs.h` → `spec/dos_structs.h`|PUBLIC INTERFACE|
|`os/milton/fileio_fat.c:49`|`dos_structs.h` → `spec/dos_structs.h`|PUBLIC INTERFACE|
|`os/milton/int21.c:24`|`dos_structs.h` → `spec/dos_structs.h`|PUBLIC INTERFACE|
|`os/milton/int21.c:25`|`tsr_contract.h` → `spec/tsr_contract.h`|PUBLIC INTERFACE|
|`os/milton/int21.c:26`|`memory_map.h` → `spec/memory_map.h`|PUBLIC INTERFACE|
|`os/milton/int21.c:27`|`find_data.h` → `spec/find_data.h`|PUBLIC INTERFACE|
|`os/milton/int21.c:30`|`dos_messages.h` → `build/dos_messages.h`|PUBLIC INTERFACE|
|`os/milton/kbd.c:18`|`../flair/ring_report.h` → `os/flair/ring_report.h`|INTERNAL LEAK|
|`os/milton/kbd.c:19`|`../../spec/tsr_contract.h` → `spec/tsr_contract.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:24`|`dos_messages.h` → `build/dos_messages.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:39`|`kmain_cold.h` → `os/milton/kmain_cold.h`|INTERNAL LEAK|
|`os/milton/kmain.c:40`|`memory_map.h` → `spec/memory_map.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:41`|`tsr_contract.h` → `spec/tsr_contract.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:43`|`../flair/surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/milton/kmain.c:60`|`command.h` → `os/milton/command.h`|INTERNAL LEAK|
|`os/milton/kmain.c:62`|`power.h` → `os/milton/power.h`|INTERNAL LEAK|
|`os/milton/kmain.c:65`|`film_startup.h` → `os/flair/film_startup.h`|INTERNAL LEAK|
|`os/milton/kmain.c:77`|`heap.h` → `os/flair/heap.h`|INTERNAL LEAK|
|`os/milton/kmain.c:78`|`surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/milton/kmain.c:79`|`cursor.h` → `os/flair/cursor.h`|INTERNAL LEAK|
|`os/milton/kmain.c:80`|`region_algebra.h` → `spec/region_algebra.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:81`|`region.h` → `os/flair/atkinson/region.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:83`|`shell.h` → `os/flair/shell.h`|INTERNAL LEAK|
|`os/milton/kmain.c:84`|`shell_menus.h` → `os/flair/shell_menus.h`|INTERNAL LEAK|
|`os/milton/kmain.c:85`|`desktop.h` → `os/flair/desktop.h`|INTERNAL LEAK|
|`os/milton/kmain.c:86`|`flair_look.h` → `os/flair/flair_look.h`|INTERNAL LEAK|
|`os/milton/kmain.c:87`|`chrome.h` → `os/flair/chrome.h`|INTERNAL LEAK|
|`os/milton/kmain.c:88`|`menu_canon.h` → `spec/assets/menu_canon.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:89`|`palette.h` → `spec/assets/palette.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:90`|`flair_dac.h` → `build/flair_dac.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:91`|`event.h` → `os/flair/event.h`|INTERNAL LEAK|
|`os/milton/kmain.c:95`|`desktop_db.h` → `os/milton/desktop_db.h`|INTERNAL LEAK|
|`os/milton/kmain.c:102`|`process.h` → `os/flair/process.h`|INTERNAL LEAK|
|`os/milton/kmain.c:104`|`system_key.h` → `os/flair/system_key.h`|INTERNAL LEAK|
|`os/milton/kmain.c:105`|`power_notice.h` → `os/flair/power_notice.h`|INTERNAL LEAK|
|`os/milton/kmain.c:106`|`egg_jtc.h` → `os/flair/egg_jtc.h`|INTERNAL LEAK|
|`os/milton/kmain.c:108`|`film_scene.h` → `os/flair/film_scene.h`|INTERNAL LEAK|
|`os/milton/kmain.c:110`|`ref_tenant.h` → `os/apps/ref_tenant.h`|INTERNAL LEAK|
|`os/milton/kmain.c:111`|`flair_tenants_demo.h` → `spec/flair_tenants_demo.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:112`|`finder_desktop.h` → `os/flair/finder_desktop.h`|INTERNAL LEAK|
|`os/milton/kmain.c:116`|`finder_cmd.h` → `os/flair/finder_cmd.h`|INTERNAL LEAK|
|`os/milton/kmain.c:119`|`finder_windows.h` → `os/flair/finder_windows.h`|INTERNAL LEAK|
|`os/milton/kmain.c:123`|`finder_ops.h` → `os/flair/finder_ops.h`|INTERNAL LEAK|
|`os/milton/kmain.c:126`|`finder_menu.h` → `os/flair/finder_menu.h`|INTERNAL LEAK|
|`os/milton/kmain.c:129`|`tbxgate.h` → `os/flair/tbxgate.h`|INTERNAL LEAK|
|`os/milton/kmain.c:131`|`apple_menu.h` → `os/flair/apple_menu.h`|INTERNAL LEAK|
|`os/milton/kmain.c:134`|`toolbox_gate.h` → `spec/toolbox_gate.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:135`|`stdfile.h` → `os/flair/stdfile.h`|INTERNAL LEAK|
|`os/milton/kmain.c:136`|`scrap.h` → `os/flair/scrap.h`|INTERNAL LEAK|
|`os/milton/kmain.c:137`|`transient.h` → `os/milton/transient.h`|INTERNAL LEAK|
|`os/milton/kmain.c:138`|`overlay_format.h` → `spec/overlay_format.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:139`|`about_box.h` → `os/milton/about_box.h`|INTERNAL LEAK|
|`os/milton/kmain.c:141`|`dos_messages.h` → `build/dos_messages.h`|PUBLIC INTERFACE|
|`os/milton/kmain.c:182`|`ref_demo_menu.h` → `os/apps/ref_demo_menu.h`|INTERNAL LEAK|
|`os/milton/kmain.c:1294`|`finder_actions.h` → `os/milton/finder_actions.h`|INTERNAL LEAK|
|`os/milton/kmain.c:1295`|`power_cold.h` → `os/milton/power_cold.h`|INTERNAL LEAK|
|`os/milton/kmain.c:5830`|`../../spec/i123_copy_smoke_keys.h` → `spec/i123_copy_smoke_keys.h`|PUBLIC INTERFACE|
|`os/milton/loader.c:24`|`memory_map.h` → `spec/memory_map.h`|PUBLIC INTERFACE|
|`os/milton/loader.c:28`|`toolbox_gate.h` → `spec/toolbox_gate.h`|PUBLIC INTERFACE|
|`os/milton/loader.c:29`|`transient.h` → `os/milton/transient.h`|INTERNAL LEAK|
|`os/milton/loader_cold.c:6`|`memory_map.h` → `spec/memory_map.h`|PUBLIC INTERFACE|
|`os/milton/loader_cold.c:17`|`transient.h` → `os/milton/transient.h`|INTERNAL LEAK|
|`os/milton/mcb.h:30`|`dos_structs.h` → `spec/dos_structs.h`|PUBLIC INTERFACE|
|`os/milton/mz.h:30`|`../../spec/initechmz_extended.h` → `spec/initechmz_extended.h`|PUBLIC INTERFACE|
|`os/milton/native.c:6`|`../../spec/initechmz_extended.h` → `spec/initechmz_extended.h`|PUBLIC INTERFACE|
|`os/milton/overlay_hdr.asm:19`|`ovl_imports_ext.inc` → `UNRESOLVED`|INTERNAL LEAK|
|`os/milton/overlay_hdr.asm:42`|`ovl_imports_dd.inc` → `UNRESOLVED`|INTERNAL LEAK|
|`os/milton/overlay_hdr.asm:43`|`ovl_exports_dd.inc` → `UNRESOLVED`|INTERNAL LEAK|
|`os/milton/psp.h:32`|`dos_structs.h` → `spec/dos_structs.h`|PUBLIC INTERFACE|
|`os/milton/sysinit.c:25`|`memory_map.h` → `spec/memory_map.h`|PUBLIC INTERFACE|
|`os/milton/sysinit.c:27`|`fpu_contract.h` → `spec/fpu_contract.h`|PUBLIC INTERFACE|
|`os/milton/sysinit.c:28`|`tsr_contract.h` → `spec/tsr_contract.h`|PUBLIC INTERFACE|

### Generated inputs

|Header|Generator inputs / recipe origin|
|---|---|
|`build/flair_dac.h`|`build/flair_dac_gen spec/assets/clut.json spec/assets/color_canon.json`; #  recipe to execute (from 'harness/emu/vesa8.mk', line 24):|
|`build/geneva12_packed_data.h`|`build/geneva12_pack spec/assets/geneva12.h`; #  recipe to execute (from 'harness/proptest/geneva12.mk', line 51):|
|`spec/assets/palette.h`|`spec/assets/palette.json build/palette_extract`; #  recipe to execute (from 'Makefile', line 12063):|
|`build/chicago_packed_data.h`|`build/chicago_pack spec/assets/chicago12.h`; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 122):|
|`build/dos_messages.h`|`spec/dos_messages.json | build`; #  recipe to execute (from 'Makefile', line 28325):|

### Checked-in generated header provenance

These generation facts supplement section 2; checked-in output is an input during the current build, not necessarily regenerated by make.

|Header|Generator|Generator inputs|
|---|---|---|
|`os/flair/asset_lz_data.h:1`|`tools/asset_lz.c:1`|spec/assets/{desk_icons,finder_icons,alert_icons,finder_small_icons,boot_logo}.h|
|`spec/assets/boot_logo.h:1`|`tools/boot_logo_gen.c:1`|compiled-in geometry; generator:4–22 names pubs/common/initech-logo.tex and docs/pubs/DESIGN-REFERENCE.md|
|`spec/block_device_contract.h:1`|`tools/gen_block_device_spec.c:1`|spec/block_device_contract.json|
|`spec/drive_letters.h:1`|`tools/gen_drive_letters.c:1`|spec/drive_letters.json|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-kernel.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/kernel_shell.bin build/kernel_flairtenants.bin
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:8857: update target 'build/kstart.o' due to: os/milton/kstart.asm Makefile spec/memory_map.h
nasm -f elf32 -DMK_KERNEL_SECTORS=384 -DMK_KERNEL_STACK_BOT=0x005F0000 -DMK_KERNEL_STACK_TOP=0x005FFFFC os/milton/kstart.asm -o build/kstart.o
harness/proptest/kernel_size_r4.mk:86: update target 'build/serial_trace_pack' due to: tools/serial_trace_pack.c
cc -std=c11 -Wall -Wextra -Werror -O2 -o build/serial_trace_pack tools/serial_trace_pack.c
Makefile:10069: update target 'build/kmain_shell.o' due to: os/milton/kmain.c os/milton/ansi_cursor.h os/milton/finder_actions.h os/milton/power_cold.h build/serial_trace_pack os/milton/kmain_cold.h os/milton/boot_info.h os/milton/io.h os/milton/console.h os/milton/idt.h os/milton/pic.h os/milton/int21.h os/milton/loader.h os/milton/test_prog.h os/milton/psp.h os/milton/sft.h os/milton/ata.h os/milton/fat12.h os/milton/fileio_fat.h os/milton/blockdev.h os/milton/kbd.h os/milton/pit.h os/milton/command.h spec/memory_map.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -DBOOT_SHELL -Ispec -Ios/milton -E os/milton/kmain.c -o build/kmain_shell.o.i
build/serial_trace_pack build/kmain_shell.o.i build/kmain_shell.o.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -DBOOT_SHELL -Ispec -Ios/milton -c build/kmain_shell.o.c -o build/kmain_shell.o
Makefile:8949: update target 'build/console.o' due to: os/milton/console.c os/milton/console.h os/milton/boot_info.h os/flair/surface.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/milton -Ios/flair -c os/milton/console.c -o build/console.o
harness/proptest/kernel_size_r4.mk:165: update target 'build/kmain_cold.o' due to: os/milton/kmain_cold.c os/milton/kmain_cold.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/milton -c os/milton/kmain_cold.c -o build/kmain_cold.o
Makefile:8866: update target 'build/surface.o' due to: os/flair/surface.c os/flair/surface.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -c os/flair/surface.c -o build/surface.o
Makefile:8953: update target 'build/idt.o' due to: os/milton/idt.c os/milton/idt.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing -fomit-frame-pointer -Ios/milton -c os/milton/idt.c -o build/idt.o
Makefile:8956: update target 'build/pic.o' due to: os/milton/pic.c os/milton/pic.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/pic.c -o build/pic.o
Makefile:8959: update target 'build/panic.o' due to: os/milton/panic.c os/milton/idt.h os/milton/io.h os/milton/console.h os/milton/panic_screen.h os/milton/boot_info.h os/milton/panic.h os/flair/film_startup.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi --param=min-pagesize=0 -Ios/milton -c os/milton/panic.c -o build/panic.o
Makefile:28325: update target 'build/dos_messages.h' due to: spec/dos_messages.json
printf '>>> codegen: %s -> %s (63 messages, deterministic)\n' "spec/dos_messages.json" "build/dos_messages.h"
python3 -c "import json; \
d=json.load(open('spec/dos_messages.json')); \
m=d['messages'] if isinstance(d,dict) and 'messages' in d else d; \
assert isinstance(m,dict), 'messages is not an object'; \
esc=lambda s: s.replace(chr(92),chr(92)*2).replace(chr(34),chr(92)+chr(34)).replace(chr(13),chr(92)+'r').replace(chr(10),chr(92)+'n'); \
ids=['MSG-DOS-%04d'%i for i in range(1,64)]; \
missing=[k for k in ids if k not in m]; \
assert not missing, 'spec missing message id(s) (DEC-13 controlled scope): %r'%missing; \
[ (_ for _ in ()).throw(AssertionError('non-string/empty text for %s'%k)) for k in ids if not (isinstance(m[k],str) and m[k]) ]; \
[ (_ for _ in ()).throw(AssertionError('non-ASCII text for %s (Rule 12)'%k)) for k in ids if not all(ord(c)<128 for c in m[k]) ]; \
L=[]; \
L.append('/*'); \
L.append(' * dos_messages.h -- InitechDOS Approved Diagnostic Message Catalogue.'); \
L.append(' *'); \
L.append(' * GENERATED -- DO NOT EDIT; edit the spec and rebuild.'); \
L.append(' *'); \
L.append(' * Source:  spec/dos_messages.json'); \
L.append(' * Ref:     ADR-0003 Appendix C / DEC-13 (controlled vocabulary).'); \
L.append(' *          %c denotes a drive-letter substitution; spacing and'); \
L.append(' *          punctuation are part of the controlled vocabulary.'); \
L.append(' */'); \
L.append('#ifndef INITECH_DOS_MESSAGES_H'); \
L.append('#define INITECH_DOS_MESSAGES_H'); \
L.append(''); \
L+=['#define %s \"%s\"'%(k.replace('-','_'),esc(m[k])) for k in ids]; \
L.append(''); \
L.append('#endif /* INITECH_DOS_MESSAGES_H */'); \
open('build/dos_messages.h','w').write(chr(10).join(L)+chr(10))" \
	|| { printf '!!! codegen FAIL: %s invalid (missing id / bad shape / non-ASCII)\n' "spec/dos_messages.json"; exit 1; }
Makefile:8967: update target 'build/int21.o' due to: os/milton/int21.c os/milton/int21.h os/milton/conedit.h os/milton/idt.h os/milton/sft.h os/milton/psp.h os/milton/mcb.h os/milton/ata.h spec/dos_structs.h build/dos_messages.h os/milton/ansi.c os/milton/ansi.h os/milton/native.c spec/initechmz_extended.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -Ibuild -c os/milton/int21.c -o build/int21.o
Makefile:8973: update target 'build/devices.o' due to: os/milton/devices.c os/milton/devices.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/devices.c -o build/devices.o
Makefile:8979: update target 'build/mcb.o' due to: os/milton/mcb.c os/milton/mcb.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -Ispec -Ios/milton -c os/milton/mcb.c -o build/mcb.o
Makefile:8985: update target 'build/psp.o' due to: os/milton/psp.c os/milton/psp.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/psp.c -o build/psp.o
Makefile:8991: update target 'build/sft.o' due to: os/milton/sft.c os/milton/sft.h os/milton/psp.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/sft.c -o build/sft.o
Makefile:8996: update target 'build/config_sys.o' due to: os/milton/config_sys.c os/milton/config_sys.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/config_sys.c -o build/config_sys.o
Makefile:9005: update target 'build/sysinit.o' due to: os/milton/sysinit.c os/milton/sysinit.h os/milton/fpu.h spec/fpu_contract.h os/milton/config_sys.h os/milton/sft.h os/milton/psp.h os/milton/int21.h os/milton/pic.h os/milton/idt.h os/milton/pit.h os/milton/kbd.h os/milton/fat12.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/sysinit.c -o build/sysinit.o
Makefile:9013: update target 'build/loader.o' due to: os/milton/loader.c os/milton/loader.h os/milton/psp.h spec/memory_map.h os/milton/int21.h os/milton/fat12.h os/milton/blockdev.h spec/dos_structs.h os/milton/panic.h os/milton/panic_screen.h os/flair/film_startup.h os/milton/loader_cold.c os/milton/loader_cold.h spec/initechmz_extended.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/loader.c -o build/loader.o
Makefile:9019: update target 'build/ata.o' due to: os/milton/ata.c os/milton/ata.h os/milton/blockdev.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/ata.c -o build/ata.o
harness/diff/fat_diff/block_device.mk:24: update target 'build/block_device.o' due to: os/milton/block_device.c os/milton/block_device.h spec/block_device_contract.h spec/drive_letters.h os/milton/diskutil.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/block_device.c -o build/block_device.o
Makefile:9025: update target 'build/fat12.o' due to: os/milton/fat12.c os/milton/fat12.h os/milton/blockdev.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/fat12.c -o build/fat12.o
Makefile:9034: update target 'build/fileio_fat.o' due to: os/milton/fileio_fat.c os/milton/fileio_fat.h os/milton/fat12.h os/milton/int21.h os/milton/blockdev.h spec/dos_structs.h spec/memory_map.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/fileio_fat.c -o build/fileio_fat.o
Makefile:9058: update target 'build/kbd.o' due to: os/milton/kbd.c os/milton/kbd.h os/milton/io.h spec/tsr_contract.h os/flair/ring_report.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing -fomit-frame-pointer -DKERNEL_PURE_DECODE_OZ -Ios/milton -c os/milton/kbd.c -o build/kbd.o
Makefile:9074: update target 'build/pit.o' due to: os/milton/pit.c os/milton/pit.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/pit.c -o build/pit.o
Makefile:9084: update target 'build/rtc.o' due to: os/milton/rtc.c os/milton/rtc.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -Ios/milton -c os/milton/rtc.c -o build/rtc.o
Makefile:9090: update target 'build/irq.o' due to: os/milton/irq.c os/milton/irq.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/irq.c -o build/irq.o
Makefile:9100: update target 'build/command.o' due to: os/milton/command.c os/milton/command.h os/milton/env.h os/milton/int21.h os/milton/batch.h spec/find_data.h spec/dos_structs.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -DCOMMAND_KERNEL_REPL -Ispec -Ios/milton -Ibuild -c os/milton/command.c -o build/command.o
Makefile:9105: update target 'build/env.o' due to: os/milton/env.c os/milton/env.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c os/milton/env.c -o build/env.o
Makefile:9111: update target 'build/batch.o' due to: os/milton/batch.c os/milton/batch.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c os/milton/batch.c -o build/batch.o
Makefile:9121: update target 'build/test_program.bin' due to: os/milton/test_program.asm
nasm -f bin os/milton/test_program.asm -o build/test_program.bin
Makefile:9116: update target 'build/bin2c' due to: tools/bin2c.c
cc -std=c11 -Wall -Wextra -Werror -o build/bin2c tools/bin2c.c
Makefile:9126: update target 'build/test_prog_blob.c' due to: build/test_program.bin build/bin2c
build/bin2c build/test_program.bin g_test_prog_image > build/test_prog_blob.c
Makefile:9129: update target 'build/test_prog_blob.o' due to: build/test_prog_blob.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c build/test_prog_blob.c -o build/test_prog_blob.o
Makefile:9134: update target 'build/type_program.bin' due to: os/milton/type_program.asm
nasm -f bin os/milton/type_program.asm -o build/type_program.bin
Makefile:9137: update target 'build/type_prog_blob.c' due to: build/type_program.bin build/bin2c
build/bin2c build/type_program.bin g_type_prog_image > build/type_prog_blob.c
Makefile:9140: update target 'build/type_prog_blob.o' due to: build/type_prog_blob.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c build/type_prog_blob.c -o build/type_prog_blob.o
Makefile:9143: update target 'build/dir_program.bin' due to: os/milton/dir_program.asm
nasm -f bin os/milton/dir_program.asm -o build/dir_program.bin
Makefile:9146: update target 'build/dir_prog_blob.c' due to: build/dir_program.bin build/bin2c
build/bin2c build/dir_program.bin g_dir_prog_image > build/dir_prog_blob.c
Makefile:9149: update target 'build/dir_prog_blob.o' due to: build/dir_prog_blob.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c build/dir_prog_blob.c -o build/dir_prog_blob.o
Makefile:9228: update target 'build/isr.o' due to: os/milton/isr.asm
nasm -f elf32 os/milton/isr.asm -o build/isr.o
Makefile:10099: update target 'build/kernel_shell.elf' due to: build/kstart.o build/kmain_shell.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/pit.o build/rtc.o build/irq.o build/command.o build/env.o build/batch.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o os/milton/kernel.ld
gcc -m32 -march=i386 -nostdlib -no-pie -flto -Oz -flto-partition=one -fno-jump-tables -ffunction-sections -fdata-sections -mregparm=3 -mrtd -freg-struct-return -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Wl,--build-id=none -Xlinker --gc-sections -T os/milton/kernel.ld -o build/kernel_shell.elf build/kstart.o build/kmain_shell.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/pit.o build/rtc.o build/irq.o build/command.o build/env.o build/batch.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o
Makefile:10102: update target 'build/kernel_shell.bin' due to: build/kernel_shell.elf
objcopy -O binary build/kernel_shell.elf build/kernel_shell.bin
sz=$(wc -c < build/kernel_shell.bin); max=$(( 384 * 512 )); \
if [ "$sz" -gt "$max" ]; then \
	printf '!!! kernel_shell.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$sz" "$max"; \
	exit 1; \
fi; \
dd if=/dev/zero of=build/kernel_shell.bin bs=1 seek="$sz" count="$(( max - sz ))" conv=notrunc status=none; \
printf ">>> kernel(shell): %s (flat binary @0x500000, padded to %d sectors)\n" "build/kernel_shell.bin" "384"
end=$(nm build/kernel_shell.elf | awk '$3=="_kernel_end"{print "0x"$1}'); lowest=$(nm -n build/kernel_shell.elf | awk '$1 ~ /^[0-9A-Fa-f]+$/ && $2 ~ /^[TtRrDdBb]$/ {print "0x"$1; exit}'); if [ -z "$end" ]; then printf '!!! kernel-end guard (shell): _kernel_end symbol not found in %s\n' "build/kernel_shell.elf"; exit 1; fi; if [ -z "$lowest" ]; then printf '!!! kernel-floor guard (shell): no defined symbol address found in %s\n' "build/kernel_shell.elf"; exit 1; fi; endv=$(( end )); lowv=$(( lowest )); base=$(( 0x00500000 )); ceil=$(( 0x00580000 )); if [ "$lowv" -lt "$base" ]; then printf '!!! kernel-floor guard (shell): lowest symbol=0x%x below KERNEL_BASE=0x%x -- LOAD_LOW relapse\n' "$lowv" "$base"; exit 1; fi; if [ "$endv" -gt "$ceil" ]; then printf '!!! kernel-end guard (shell): _kernel_end=0x%x passes KERNEL_IMAGE_CEIL=0x%x (transient base; kernel stack above) -- shrink the kernel or re-ratify the runway sub-windows; bump KERNEL_SECTORS too if loaded bytes grew\n' "$endv" "$ceil"; exit 1; fi; printf '>>> kernel placement guard (shell): lowest=0x%x >= KERNEL_BASE=0x%x; _kernel_end=0x%x <= KERNEL_IMAGE_CEIL=0x%x OK\n' "$lowv" "$base" "$endv" "$ceil"
Makefile:10832: update target 'build/loader_tenant.o' due to: os/milton/loader.c os/milton/loader_cold.h os/milton/loader.h os/milton/mz.c spec/initechmz_extended.h spec/memory_map.h os/milton/loader_cold.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DLOADER_COLD_EXTERNAL -Ispec -Ios/milton -c os/milton/loader.c -o build/loader_tenant.o
Makefile:9064: update target 'build/mouse.o' due to: os/milton/mouse.c os/milton/mouse.h os/milton/mouse_pack.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/mouse.c -o build/mouse.o
Makefile:8887: update target 'build/region.o' due to: os/flair/atkinson/region.c os/flair/atkinson/region.h spec/region_algebra.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -Ispec -Ios/flair/atkinson -Iseed -c os/flair/atkinson/region.c -o build/region.o
Makefile:8890: update target 'build/heap.o' due to: os/flair/heap.c os/flair/heap.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Iseed -c os/flair/heap.c -o build/heap.o
Makefile:8893: update target 'build/event.o' due to: os/flair/event.c os/flair/event.h spec/event_model.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -Ios/flair -Ispec -Iseed -c os/flair/event.c -o build/event.o
Makefile:8896: update target 'build/cursor.o' due to: os/flair/cursor.c os/flair/cursor.h os/flair/surface.h spec/assets/cursors.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ispec/assets -c os/flair/cursor.c -o build/cursor.o
Makefile:8907: update target 'build/window_live.o' due to: os/flair/window.c os/flair/window.h os/flair/winscroll.h os/flair/atkinson/region.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/chrome_metrics.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DWINDOW_ENABLE_R1_OPS -Ispec -Ios/flair -Ios/flair/atkinson -Iseed -c os/flair/window.c -o build/window_live.o
Makefile:8910: update target 'build/blitter.o' due to: os/flair/blitter.c os/flair/blitter.h os/flair/atkinson/region.h os/flair/surface.h spec/region_algebra.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -Ispec -Ios/flair -Ios/flair/atkinson -Iseed -c os/flair/blitter.c -o build/blitter.o
Makefile:12043: update target 'build/palette_extract' due to: tools/palette_extract.c
cc -std=c11 -Wall -Wextra -Werror -o build/palette_extract tools/palette_extract.c
Makefile:12063: update target 'spec/assets/palette.h' due to: spec/assets/palette.json build/palette_extract
build/palette_extract --header spec/assets/palette.json > spec/assets/palette.h.tmp.$$ \
	&& { if cmp -s spec/assets/palette.h.tmp.$$ spec/assets/palette.h; then rm -f spec/assets/palette.h.tmp.$$; \
	     else mv -f spec/assets/palette.h.tmp.$$ spec/assets/palette.h && printf ">>> assets: regenerated %s from %s\n" "spec/assets/palette.h" "spec/assets/palette.json"; fi; } \
	|| { rm -f spec/assets/palette.h.tmp.$$; exit 1; }
Makefile:8918: update target 'build/chrome.o' due to: os/flair/chrome.c os/flair/chrome.h os/flair/winscroll.h os/flair/text.h spec/assets/font_linkage.h spec/assets/chicago12.h spec/assets/geneva9.h spec/assets/geneva12.h os/flair/flair_look.h spec/chrome_metrics.h spec/grafport.h spec/imaging.h spec/region_algebra.h spec/assets/palette.h os/flair/rdoc.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -c os/flair/chrome.c -o build/chrome.o
harness/proptest/kernel_size_r4.mk:120: update target 'build/chicago_pack' due to: tools/chicago_pack.c spec/assets/chicago12.h
cc -std=c11 -Wall -Wextra -Werror -Ispec/assets -o build/chicago_pack tools/chicago_pack.c
harness/proptest/kernel_size_r4.mk:122: update target 'build/chicago_packed_data.h' due to: build/chicago_pack spec/assets/chicago12.h
build/chicago_pack >build/chicago_packed_data.h.tmp && mv -f build/chicago_packed_data.h.tmp build/chicago_packed_data.h
harness/proptest/geneva12.mk:49: update target 'build/geneva12_pack' due to: tools/chicago_pack.c spec/assets/geneva12.h spec/assets/font_linkage.h
cc -std=c11 -Wall -Wextra -Werror -DPACK_GENEVA12 -Ispec/assets -o build/geneva12_pack tools/chicago_pack.c
harness/proptest/geneva12.mk:51: update target 'build/geneva12_packed_data.h' due to: build/geneva12_pack spec/assets/geneva12.h
build/geneva12_pack >build/geneva12_packed_data.h.tmp && mv -f build/geneva12_packed_data.h.tmp build/geneva12_packed_data.h
Makefile:8921: update target 'build/text.o' due to: os/flair/text.c os/flair/text.h spec/assets/font_linkage.h spec/assets/chicago12.h spec/assets/geneva9.h spec/assets/geneva12.h os/flair/surface.h os/flair/chicago_packed.h build/chicago_packed_data.h os/flair/geneva12_packed.h build/geneva12_packed_data.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/flair -Ispec/assets -Ispec -Iseed -c os/flair/text.c -o build/text.o
Makefile:8927: update target 'build/menu.o' due to: os/flair/menu.c os/flair/menu.h os/flair/flair_look.h spec/assets/menu_canon.h spec/assets/apple_glyph.h spec/chrome_metrics.h spec/grafport.h spec/imaging.h spec/region_algebra.h spec/assets/palette.h spec/assets/chicago12.h spec/assets/geneva9.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/menu.c -o build/menu.o
Makefile:8930: update target 'build/control.o' due to: os/flair/control.c os/flair/control.h os/flair/flair_look.h spec/chrome_metrics.h spec/grafport.h spec/imaging.h spec/region_algebra.h spec/assets/palette.h spec/assets/chicago12.h spec/progress_platinum.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/control.c -o build/control.o
Makefile:8933: update target 'build/dialog.o' due to: os/flair/dialog.c os/flair/dialog.h spec/assets/alert_icons.h spec/assets/desk_icons.h os/flair/flair_look.h spec/chrome_metrics.h spec/grafport.h spec/event_model.h spec/window_record.h spec/region_algebra.h spec/assets/palette.h spec/assets/chicago12.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/dialog.c -o build/dialog.o
Makefile:8936: update target 'build/desktop.o' due to: os/flair/desktop.c os/flair/desktop.h os/flair/window.h os/flair/winscroll.h os/flair/event.h os/flair/blitter.h os/flair/chrome.h os/flair/surface.h os/flair/heap.h os/flair/atkinson/region.h spec/region_algebra.h spec/window_record.h spec/event_model.h spec/grafport.h spec/imaging.h spec/chrome_metrics.h spec/assets/palette.h os/flair/flair_look.h os/flair/rdoc.h spec/cpanels_record.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -c os/flair/desktop.c -o build/desktop.o
Makefile:8940: update target 'build/flair_look.o' due to: os/flair/flair_look.c os/flair/flair_look.h spec/flair_skins.h spec/assets/color_canon.h os/flair/surface.h spec/grafport.h spec/imaging.h spec/assets/finder_small_icons.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/flair_look.c -o build/flair_look.o
Makefile:8943: update target 'build/flair_shell.o' due to: os/flair/shell.c os/flair/shell.h os/flair/text.h spec/assets/font_linkage.h spec/assets/chicago12.h spec/assets/geneva9.h spec/assets/geneva12.h os/flair/desktop.h os/flair/window.h os/flair/winscroll.h os/flair/menu.h os/flair/dialog.h os/flair/control.h os/flair/chrome.h os/flair/blitter.h os/flair/surface.h os/flair/heap.h os/flair/event.h os/flair/atkinson/region.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/imaging.h spec/chrome_metrics.h spec/assets/menu_canon.h spec/assets/palette.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -c os/flair/shell.c -o build/flair_shell.o
harness/emu/vesa8.mk:22: update target 'build/flair_dac_gen' due to: tools/flair_dac_gen.c
cc -std=c11 -Wall -Wextra -Werror -O2 tools/flair_dac_gen.c -o build/flair_dac_gen
harness/emu/vesa8.mk:24: update target 'build/flair_dac.h' due to: build/flair_dac_gen spec/assets/clut.json spec/assets/color_canon.json
build/flair_dac_gen spec/assets/clut.json spec/assets/color_canon.json > build/flair_dac.h.tmp
mv -f build/flair_dac.h.tmp build/flair_dac.h
Makefile:10814: update target 'build/kmain_flairtenants.o' due to: os/milton/kmain.c os/milton/ansi_cursor.h os/milton/finder_actions.h os/milton/power_cold.h build/serial_trace_pack os/milton/kmain_cold.h os/apps/ref_demo_menu.h os/milton/boot_info.h os/milton/io.h os/milton/console.h os/milton/idt.h os/milton/pic.h os/milton/int21.h os/milton/loader.h os/milton/test_prog.h os/milton/psp.h os/milton/sft.h os/milton/ata.h os/milton/fat12.h os/milton/fileio_fat.h os/milton/desktop_db.h os/milton/blockdev.h os/milton/kbd.h os/milton/mouse.h os/milton/mouse_pack.h os/milton/pit.h os/milton/sysinit.h os/milton/command.h os/flair/heap.h os/flair/surface.h os/flair/shell.h os/flair/desktop.h os/flair/window.h os/flair/winscroll.h os/flair/menu.h os/flair/dialog.h os/flair/control.h os/flair/event.h os/flair/system_key.h os/flair/power_notice.h os/flair/process.h os/flair/finder_desktop.h os/flair/finder_cmd.h os/flair/finder_windows.h os/flair/finder_ops.h os/flair/finder_menu.h os/apps/ref_tenant.h spec/event_model.h spec/memory_map.h spec/dos_structs.h spec/region_algebra.h spec/flair_tenants_demo.h spec/assets/desk_icons.h spec/assets/finder_icons.h spec/assets/menu_canon.h spec/assets/palette.h spec/assets/color_canon.h os/flair/apple_menu.h os/flair/tbxgate.h spec/toolbox_gate.h os/flair/scrap.h os/flair/stdfile.h os/flair/list.h os/flair/text.h os/milton/transient.h build/dos_messages.h os/flair/cursor.h os/milton/boot_screen.h spec/assets/boot_logo.h spec/assets/chicago12.h build/flair_dac.h os/milton/about_box.h os/flair/egg_jtc.h os/flair/egg_jtc_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/apps -Ios/milton -E os/milton/kmain.c -o build/kmain_flairtenants.o.i
build/serial_trace_pack build/kmain_flairtenants.o.i build/kmain_flairtenants.o.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/apps -Ios/milton -c build/kmain_flairtenants.o.c -o build/kmain_flairtenants.o
Makefile:10443: update target 'build/process.o' due to: os/flair/process.c os/flair/process.h os/flair/window.h os/flair/winscroll.h os/flair/heap.h os/flair/surface.h os/flair/event.h spec/event_model.h spec/region_algebra.h spec/window_record.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/process.c -o build/process.o
Makefile:10449: update target 'build/ref_tenant.o' due to: os/apps/ref_tenant.c os/apps/ref_tenant.h os/flair/process.h os/flair/window.h os/flair/winscroll.h os/flair/blitter.h os/flair/surface.h os/flair/heap.h os/flair/flair_look.h spec/window_record.h spec/region_algebra.h spec/event_model.h spec/chrome_metrics.h spec/flair_tenants_demo.h spec/assets/color_canon.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/apps -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/apps/ref_tenant.c -o build/ref_tenant.o
Makefile:9040: update target 'build/desktop_db.o' due to: os/milton/desktop_db.c os/milton/desktop_db.h os/milton/fat12.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/desktop_db.c -o build/desktop_db.o
Makefile:10458: update target 'build/finder_icon.o' due to: os/flair/finder_icon.c os/flair/finder_icon.h os/flair/flair_look.h os/flair/surface.h spec/assets/desk_icons.h spec/region_algebra.h spec/assets/finder_small_icons.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_icon.c -o build/finder_icon.o
Makefile:10462: update target 'build/finder_desktop.o' due to: os/flair/finder_desktop.c os/flair/finder_desktop.h os/flair/finder_icon.h os/flair/window.h os/flair/winscroll.h os/flair/process.h os/flair/heap.h os/flair/blitter.h os/flair/text.h os/flair/surface.h os/flair/flair_look.h spec/assets/desk_icons.h spec/assets/finder_icons.h spec/assets/geneva9.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/event_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_desktop.c -o build/finder_desktop.o
Makefile:10470: update target 'build/finder_windows.o' due to: os/flair/finder_windows.c os/flair/finder_windows_cold.c os/flair/finder_windows_private.h os/flair/finder_windows.h os/flair/finder_desktop.h os/flair/finder_cmd.h os/flair/finder_icon.h os/flair/window.h os/flair/winscroll.h os/flair/process.h os/flair/heap.h os/flair/blitter.h os/flair/surface.h os/flair/flair_look.h spec/assets/desk_icons.h spec/assets/finder_icons.h spec/assets/chicago12.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/event_model.h spec/assets/finder_list_titles.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -DFINDER_OVL_RESIDENT -c os/flair/finder_windows.c -o build/finder_windows.o
Makefile:10479: update target 'build/finder_ops.o' due to: os/flair/finder_ops.c os/flair/finder_ops_cold.c os/flair/finder_ops_private.h os/flair/finder_ops.h os/flair/finder_windows.h os/flair/finder_desktop.h os/flair/finder_cmd.h os/flair/window.h os/flair/winscroll.h os/flair/process.h os/flair/heap.h os/flair/surface.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/event_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -DFINDER_OVL_RESIDENT -c os/flair/finder_ops.c -o build/finder_ops.o
Makefile:10486: update target 'build/finder_cmd.o' due to: os/flair/finder_cmd.c os/flair/finder_cmd.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -c os/flair/finder_cmd.c -o build/finder_cmd.o
Makefile:10495: update target 'build/finder_menu.o' due to: os/flair/finder_menu.c os/flair/finder_menu.h os/flair/finder_cmd.h os/flair/menu.h os/flair/text.h os/flair/blitter.h os/flair/surface.h spec/chrome_metrics.h spec/grafport.h spec/region_algebra.h spec/assets/chicago12.h os/flair/finder_windows.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_menu.c -o build/finder_menu.o
Makefile:10424: update target 'build/tbxgate.o' due to: build/serial_trace_pack spec/find_data.h os/flair/cp_service.h spec/cp_dialog.h spec/cpanels_record.h os/flair/scrap.h os/flair/ostype.h os/flair/menu.h os/flair/tbxmenu.h spec/grafport.h os/flair/text.h os/flair/tbxgate.c os/flair/tbxgate.h os/flair/process.h os/flair/window.h os/flair/winscroll.h os/flair/heap.h os/flair/blitter.h os/flair/surface.h os/flair/flair_look.h os/flair/event.h os/milton/loader.h os/milton/psp.h spec/toolbox_gate.h spec/event_model.h spec/region_algebra.h spec/window_record.h spec/chrome_metrics.h spec/assets/chicago12.h spec/assets/geneva12.h os/flair/tbxfile.h os/milton/int21.h os/milton/sft.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ios/milton -Ispec -Ispec/assets -Dtputs=serial_puts -Dtputu=serial_putu -Dtputi=serial_puti -E os/flair/tbxgate.c -o build/tbxgate.o.i
build/serial_trace_pack build/tbxgate.o.i build/tbxgate.o.c tbx
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ios/milton -Ispec -Ispec/assets -c build/tbxgate.o.c -o build/tbxgate.o
Makefile:10426: update target 'build/tbx_gate.o' due to: os/milton/tbx_gate.asm
nasm -f elf32 os/milton/tbx_gate.asm -o build/tbx_gate.o
Makefile:10435: update target 'build/scrap.o' due to: os/flair/scrap.c os/flair/scrap.h os/flair/ostype.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ispec -c os/flair/scrap.c -o build/scrap.o
Makefile:10437: update target 'build/stdfile.o' due to: os/flair/stdfile.c os/flair/stdfile.h os/flair/list.h spec/region_algebra.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -c os/flair/stdfile.c -o build/stdfile.o
Makefile:10439: update target 'build/list.o' due to: os/flair/list.c os/flair/list.h spec/region_algebra.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -c os/flair/list.c -o build/list.o
Makefile:10822: update target 'build/apple_menu.o' due to: os/flair/apple_menu.c os/flair/apple_menu.h os/flair/menu.h spec/apple_menu_items.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/apple_menu.c -o build/apple_menu.o
harness/proptest/egg_jtc.mk:53: update target 'build/egg_jtc_model.o' due to: os/flair/egg_jtc_model.c os/flair/egg_jtc_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -Ibuild -c os/flair/egg_jtc_model.c -o build/egg_jtc_model.o
harness/proptest/egg_jtc.mk:6: update target 'build/egg_jtc_pack' due to: tools/egg_jtc_pack.c os/flair/egg_jtc_model.h
cc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -Ibuild -o build/egg_jtc_pack tools/egg_jtc_pack.c
harness/proptest/egg_jtc.mk:8: update target 'build/egg_jtc_data.bin' due to: spec/easter/jump_to_conclusions.json build/egg_jtc_pack
build/egg_jtc_pack spec/easter/jump_to_conclusions.json > build/egg_jtc_data.bin
harness/proptest/egg_jtc.mk:10: update target 'build/egg_jtc_data.c' due to: build/egg_jtc_data.bin build/bin2c
build/bin2c build/egg_jtc_data.bin egg_jtc_data > build/egg_jtc_data.c
harness/proptest/egg_jtc.mk:57: update target 'build/egg_jtc_data.o' due to: build/egg_jtc_data.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -c build/egg_jtc_data.c -o build/egg_jtc_data.o
harness/proptest/egg_jtc.mk:45: update target 'build/egg_jtc.o' due to: os/flair/egg_jtc.c os/flair/egg_jtc.h os/flair/egg_jtc_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -Ibuild -c os/flair/egg_jtc.c -o build/egg_jtc.o
harness/proptest/egg_jtc.mk:48: update target 'build/about_box_ovl.o' due to: os/milton/about_box.c os/milton/about_box.h os/milton/kmain_cold.h os/milton/int21.h os/milton/boot_info.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/milton -Ispec -c os/milton/about_box.c -o build/about_box_ovl.o
harness/proptest/egg_jtc.mk:50: update target 'build/kmain_cold_ovl.o' due to: os/milton/kmain_cold.c os/milton/kmain_cold.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/milton -c os/milton/kmain_cold.c -o build/kmain_cold_ovl.o
Makefile:10854: update target 'build/finder_actions_ovl.o' due to: os/milton/finder_actions.c os/milton/finder_actions.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/milton -c os/milton/finder_actions.c -o build/finder_actions_ovl.o
Makefile:10856: update target 'build/finder_ops_ovl.o' due to: os/flair/finder_ops_cold.c os/flair/finder_ops_private.h os/flair/finder_ops.h os/flair/finder_windows.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_ops_cold.c -o build/finder_ops_ovl.o
Makefile:10858: update target 'build/finder_windows_ovl.o' due to: os/flair/finder_windows_cold.c os/flair/finder_windows_private.h os/flair/finder_windows.h spec/assets/chicago12.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_windows_cold.c -o build/finder_windows_ovl.o
Makefile:10849: update target 'build/power_cold_ovl.o' due to: os/milton/power_cold.c os/milton/power_cold.h os/milton/finder_actions.h os/milton/power.h os/flair/power_notice.h os/milton/transient.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/milton -c os/milton/power_cold.c -o build/power_cold_ovl.o
Makefile:10851: update target 'build/loader_cold_ovl.o' due to: os/milton/loader_cold.c os/milton/loader_cold.h os/milton/mz.c os/milton/mz.h spec/initechmz_extended.h spec/memory_map.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -DLOADER_COLD_EXTERNAL -Ispec -Ios/milton -c os/milton/loader_cold.c -o build/loader_cold_ovl.o
Makefile:10985: update target 'build/transient_imports.rsp' due to: build/command.o build/env.o build/batch.o build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o build/egg_jtc.o build/about_box_ovl.o build/kmain_cold_ovl.o build/finder_actions_ovl.o build/finder_ops_ovl.o build/finder_windows_ovl.o build/power_cold_ovl.o build/loader_cold_ovl.o tools/mk_ovl_tables.sh
export LC_ALL=C; \
for o in build/command.o build/env.o build/batch.o; do gcc-nm -u $o 2>/dev/null | awk '{print $NF}'; done | grep -v ':$' | sort -u > build/transient_imports.rsp.und; \
for o in build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o; do gcc-nm --defined-only $o 2>/dev/null | awk 'NF==3{print $3}'; done | sort -u > build/transient_imports.rsp.kdef; \
for o in build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o; do gcc-nm -u $o 2>/dev/null | awk '{print $NF}'; done | grep -v ':$' | sort -u > build/transient_imports.rsp.kund; \
grep -xF -f build/transient_imports.rsp.kdef build/transient_imports.rsp.und > build/transient_imports.rsp.keep || true; \
n=$(wc -l < build/transient_imports.rsp.keep); [ "$n" -gt 0 ] || { printf '!!! transient imports: no kernel symbol kept -- the derivation is broken\n'; exit 1; }; \
sed 's/^/-Wl,--undefined=/' build/transient_imports.rsp.keep > build/transient_imports.rsp; \
{ sed 's/^/extern /' build/transient_imports.rsp.keep; printf '%%define TRANSIENT_N_IMPORTS %s\n' "$n"; } > build/transient_imports_ext.inc; \
{ sed '$d' build/transient_imports.rsp.keep | sed 's/^/    dd /'; printf '    dd %s + TRANSIENT_LAST_IMPORT_SKEW\n' "$(tail -n 1 build/transient_imports.rsp.keep)"; } > build/transient_imports_dd.inc; \
{ printf 'bits 32\n'; sed 's/^/extern /' build/transient_imports.rsp.keep; \
  printf 'section .rodata progbits alloc noexec nowrite align=4\nglobal transient_import_want\nglobal transient_import_count\ntransient_import_want:\n'; \
  sed 's/^/    dd /' build/transient_imports.rsp.keep; printf 'transient_import_count:\n    dd %s\nsection .note.GNU-stack noalloc noexec nowrite progbits\n' "$n"; } > build/transient_want.asm; \
nasm -f elf32 build/transient_want.asm -o build/transient_want.o || exit 1; \
printf '%s\n' 'build/transient_want.o' >> build/transient_imports.rsp; \
printf '>>> transient imports: %s kernel symbols kept (%s undefined in the transient); header table + kernel want table generated\n' "$n" "$(wc -l < build/transient_imports.rsp.und)"; \
nm --defined-only build/transient_want.o | awk 'NF==3{print $3}' | sort -u - build/transient_imports.rsp.kdef -o build/transient_imports.rsp.kdef; \
sh tools/mk_ovl_tables.sh build nasm build/transient_imports.rsp.kdef build/transient_imports.rsp.kund build/egg_jtc.o build/about_box_ovl.o build/kmain_cold_ovl.o build/finder_actions_ovl.o build/finder_ops_ovl.o build/finder_windows_ovl.o build/power_cold_ovl.o build/loader_cold_ovl.o || exit 1; \
cat build/ovl_undefined.rsp >> build/transient_imports.rsp; \
printf '%s\n' 'build/ovl_kernel.o' >> build/transient_imports.rsp
Makefile:10903: update target 'build/kernel_flairtenants.elf' due to: build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o os/milton/kernel.ld build/transient_imports.rsp
gcc -m32 -march=i386 -nostdlib -no-pie -flto -Oz -flto-partition=one -fno-jump-tables -ffunction-sections -fdata-sections -mregparm=3 -mrtd -freg-struct-return -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Wl,--build-id=none -Xlinker --gc-sections -T os/milton/kernel.ld -o build/kernel_flairtenants.elf build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o @build/transient_imports.rsp
Makefile:10906: update target 'build/kernel_flairtenants.bin' due to: build/kernel_flairtenants.elf
objcopy -O binary build/kernel_flairtenants.elf build/kernel_flairtenants.bin
sz=$(wc -c < build/kernel_flairtenants.bin); max=$(( 384 * 512 )); \
if [ "$sz" -gt "$max" ]; then \
	printf '!!! kernel_flairtenants.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$sz" "$max"; \
	exit 1; \
fi; \
dd if=/dev/zero of=build/kernel_flairtenants.bin bs=1 seek="$sz" count="$(( max - sz ))" conv=notrunc status=none; \
printf ">>> kernel(flairtenants): %s (flat binary @0x500000, padded to %d sectors)\n" "build/kernel_flairtenants.bin" "384"
end=$(nm build/kernel_flairtenants.elf | awk '$3=="_kernel_end"{print "0x"$1}'); lowest=$(nm -n build/kernel_flairtenants.elf | awk '$1 ~ /^[0-9A-Fa-f]+$/ && $2 ~ /^[TtRrDdBb]$/ {print "0x"$1; exit}'); if [ -z "$end" ]; then printf '!!! kernel-end guard (flairtenants): _kernel_end symbol not found in %s\n' "build/kernel_flairtenants.elf"; exit 1; fi; if [ -z "$lowest" ]; then printf '!!! kernel-floor guard (flairtenants): no defined symbol address found in %s\n' "build/kernel_flairtenants.elf"; exit 1; fi; endv=$(( end )); lowv=$(( lowest )); base=$(( 0x00500000 )); ceil=$(( 0x00580000 )); if [ "$lowv" -lt "$base" ]; then printf '!!! kernel-floor guard (flairtenants): lowest symbol=0x%x below KERNEL_BASE=0x%x -- LOAD_LOW relapse\n' "$lowv" "$base"; exit 1; fi; if [ "$endv" -gt "$ceil" ]; then printf '!!! kernel-end guard (flairtenants): _kernel_end=0x%x passes KERNEL_IMAGE_CEIL=0x%x (transient base; kernel stack above) -- shrink the kernel or re-ratify the runway sub-windows; bump KERNEL_SECTORS too if loaded bytes grew\n' "$endv" "$ceil"; exit 1; fi; printf '>>> kernel placement guard (flairtenants): lowest=0x%x >= KERNEL_BASE=0x%x; _kernel_end=0x%x <= KERNEL_IMAGE_CEIL=0x%x OK\n' "$lowv" "$base" "$endv" "$ceil"

# stderr

```

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 14; UNIT 59; META 0; SIZE/LAYOUT 3.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-kernel.md](gates-kernel.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.

### harness/conkeys/test_conkeys.c — UNIT

`harness/conkeys/test_conkeys.c:1`; Independent manual vectors drive the real INT 21h editor, not a model of it.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_conkeys`, `build/test_conkeys_DEL`, `build/test_conkeys_ESC`, `build/test_conkeys_F1`, `build/test_conkeys_F2`, `build/test_conkeys_F3`, `build/test_conkeys_F4`, `build/test_conkeys_F5`, `build/test_conkeys_INS`, `build/test_conkeys_PAUSE`, `build/test_conkeys_PRN`.
Run/assert gate(s): `test-conkeys-mutant`, `test-conkeys-unit`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_conkeys`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild -o build/test_conkeys harness/conkeys/test_conkeys.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/kbd.c
```

### harness/diff/fat_diff/dos_parity_host.c — UNIT

`harness/diff/fat_diff/dos_parity_host.c:1`; Factory oracle for dwbl. IBM DOS 3.30 Technical Reference pp.5-12/13,. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/dos_copy_stamp_host_mut`, `build/dos_parity_host`, `build/dos_parity_host_mut`.
Run/assert gate(s): `test-dir-args`, `test-dos-stamp`, `test-dos-stamp-copy`, `test-dos-stamp-copy-emu-mutant`, `test-dos-stamp-copy-host`, `test-dos-stamp-copy-host-mutant`, `test-dos-stamp-emu-mutant`, `test-dos-stamp-host`, `test-dos-stamp-mutant`, `test-path-command`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/copy_stamp.sh`, `harness/diff/fat_diff/dos_parity.sh`, `spec/dos_structs.h`, `spec/find_data.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`, `harness/diff/fat_diff/blockdev_file.h`, `spec/dos_structs.h`, `spec/find_data.h`.
Canonical `make -n` compile/link (`build/dos_parity_host`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -no-pie -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -Ibuild -o build/dos_parity_host harness/diff/fat_diff/dos_parity_host.c os/milton/int21.c os/milton/devices.c os/milton/fileio_fat.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/sft.c os/milton/psp.c os/milton/mcb.c os/milton/irq.c
```

### harness/diff/fat_diff/dos_resident_break_check.c — UNIT

`harness/diff/fat_diff/dos_resident_break_check.c:1`; initech-c80y. Independent audit HUGE.TXT bytes + DOS 3.3 Guide p.159.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/dos_resident_break_check`.
Run/assert gate(s): `test-dos-resident-boot-write-mutant`, `test-dos-resident-break-emu`, `test-dos-resident-break-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/dos_resident_break.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/dos_resident_break_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror harness/diff/fat_diff/dos_resident_break_check.c -o build/dos_resident_break_check
```

### harness/diff/fat_diff/dos_volumes_host.c — ORACLE-STRONG

`harness/diff/fat_diff/dos_volumes_host.c:1`; Factory differential, real INT21 + FAT backend on identical mtools images.. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/dos_volumes_host`.
Run/assert gate(s): `test-dos-volumes-host`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/dos_volumes.sh`, `spec/dos_structs.h`, `spec/find_data.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`, `harness/diff/fat_diff/blockdev_file.h`, `spec/dos_structs.h`, `spec/find_data.h`.
Canonical `make -n` compile/link (`build/dos_volumes_host`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -no-pie -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -Ibuild harness/diff/fat_diff/dos_volumes_host.c os/milton/int21.c os/milton/devices.c os/milton/fileio_fat.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/sft.c os/milton/psp.c os/milton/mcb.c os/milton/irq.c -o build/dos_volumes_host
```

### harness/diff/fat_diff/fat12_corrupt_fuzz.c — ORACLE-STRONG

`harness/diff/fat_diff/fat12_corrupt_fuzz.c:1`; - deterministic, seeded CORRUPTION. property/fuzz invariants over generated inputs.
Build target(s): `build/fat12_corrupt_fuzz`, `build/fat12_corrupt_fuzz_mut_bpb`, `build/fat12_corrupt_fuzz_mut_step`, `build/ksize7/fat12_corrupt_fuzz_mut_chain`.
Run/assert gate(s): `test-fat-corrupt-fuzz`, `test-fat-corrupt-fuzz-chain-mutant`, `test-fat-corrupt-fuzz-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_corrupt_fuzz.c`, `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/fat12_corrupt_fuzz`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/fat12_corrupt_fuzz harness/diff/fat_diff/fat12_corrupt_fuzz.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/fat12_fuzz.c — ORACLE-STRONG

`harness/diff/fat_diff/fat12_fuzz.c:1`; - deterministic, seeded, shrinking. property/fuzz invariants over generated inputs.
Build target(s): `build/fat12_fuzz`, `build/fat12_fuzz_mut_rmw`, `build/fat12_fuzz_mut_stale_gap`.
Run/assert gate(s): `test-fat-fuzz`, `test-fat-fuzz-gap-mutant`, `test-fat-fuzz-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_fuzz.c`, `harness/diff/fat_diff/fat12_ref.py`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/fat12_fuzz`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/fat12_fuzz harness/diff/fat_diff/fat12_fuzz.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/int21_coverage.c — UNIT

`harness/diff/fat_diff/int21_coverage.c:1`; Factory coverage certificate: the locked register is the sole scope source.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/int21_coverage`.
Run/assert gate(s): `test-kernel-coverage`, `test-kernel-coverage-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/int21h_register.json`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/int21_coverage`):
```sh
cc -std=c11 -Wall -Wextra -Werror -O2 harness/diff/fat_diff/int21_coverage.c -o build/int21_coverage
```

### harness/diff/fat_diff/int21_dpb_host.c — UNIT

`harness/diff/fat_diff/int21_dpb_host.c:1`; Factory driver: real INT21 AH=1Fh/32h over the real FAT backend on the. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/int21_dpb_host`, `build/int21_dpb_host_first_data`.
Run/assert gate(s): `test-int21-dpb`, `test-int21-dpb-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/int21_dpb.sh`, `spec/dos_structs.h`, `spec/find_data.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`, `harness/diff/fat_diff/blockdev_file.h`, `spec/dos_structs.h`, `spec/find_data.h`.
Canonical `make -n` compile/link (`build/int21_dpb_host`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -no-pie -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -Ibuild harness/diff/fat_diff/int21_dpb_host.c os/milton/int21.c os/milton/devices.c os/milton/fileio_fat.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/sft.c os/milton/psp.c os/milton/mcb.c os/milton/irq.c -o build/int21_dpb_host
```

### harness/diff/fat_diff/test_absdisk.c — UNIT

`harness/diff/fat_diff/test_absdisk.c:1`; - INT 25h/26h ABSOLUTE DISK oracle.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_absdisk`, `build/test_absdisk_mut_errclass`, `build/test_absdisk_mut_lba`, `build/test_absdisk_mut_neighbor`, `build/test_absdisk_mut_nobnd`, `build/test_absdisk_mut_noop`, `build/test_absdisk_mut_packet`, `build/test_absdisk_mut_trans`, `build/test_absdisk_mut_wart`.
Run/assert gate(s): `test-absdisk`, `test-absdisk-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/test_absdisk.c`, `spec/absdisk_int2526.json`, `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`, `harness/diff/fat_diff/blockdev_file.h`, `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_absdisk`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -Ibuild \
	-o build/test_absdisk harness/diff/fat_diff/test_absdisk.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_block_device.c — UNIT

`harness/diff/fat_diff/test_block_device.c:1`; Independent BIOS arithmetic/register oracle. IBM DOS TR pp9-8..9;. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_block_device`, `build/test_block_device_CARRY`, `build/test_block_device_HEADS`, `build/test_block_device_WATCH`.
Run/assert gate(s): `test-block-device`, `test-block-device-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/block_device_contract.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/block_device_contract.h`.
Canonical `make -n` compile/link (`build/test_block_device`):
```sh
cc -std=c11 -O2 -Wall -Wextra -Werror -no-pie -DBLOCK_DEVICE_HOST -Ios/milton -Ispec harness/diff/fat_diff/test_block_device.c os/milton/block_device.c -o build/test_block_device
```

### harness/diff/fat_diff/test_console_look.c — UNIT

`harness/diff/fat_diff/test_console_look.c:1`; uint8_t *fb = mmap(NULL, 640*480*4, PROT_READ\|PROT_WRITE,. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_console_look`.
Run/assert gate(s): `test-console-look`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_console_look`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec -Ios/milton -Ibuild -Iharness/diff/fat_diff -o build/test_console_look harness/diff/fat_diff/test_console_look.c os/milton/console.c os/flair/surface.c
```

### harness/diff/fat_diff/test_dos_break.c — UNIT

`harness/diff/fat_diff/test_dos_break.c:1`; initech-vlhp: DEC-16 Sec 3.3/C-6; MS-DOS 3.3 Reference p.38. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_dos_break`, `build/test_dos_break_cleanup_mut`, `build/test_dos_break_mut`.
Run/assert gate(s): `test-dos-break`, `test-dos-break-cleanup-mutant`, `test-dos-break-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_dos_break`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -no-pie -Ispec -Ios/milton -Ibuild harness/diff/fat_diff/test_dos_break.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/kbd.c -o build/test_dos_break
```

### harness/diff/fat_diff/test_dos_not_ready.c — UNIT

`harness/diff/fat_diff/test_dos_not_ready.c:1`; vlhp: MS-DOS 3.3 Reference Appendix F p.350/PDF435; ADR-0003. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_dos_not_ready`, `build/test_dos_not_ready_mut`.
Run/assert gate(s): `test-dos-not-ready`, `test-dos-not-ready-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`, `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_dos_not_ready`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iharness/diff/fat_diff -Ibuild harness/diff/fat_diff/test_dos_not_ready.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/ata.c harness/diff/fat_diff/blockdev_file.c -o build/test_dos_not_ready
```

### harness/diff/fat_diff/test_drive_letters.c — UNIT

`harness/diff/fat_diff/test_drive_letters.c:1`; Independent literals from the PM DEC-18 ruling, not generated answers.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_drive_letters`, `build/test_drive_letters_DISK`, `build/test_drive_letters_RUNNING`.
Run/assert gate(s): `test-drive-letters`, `test-drive-letters-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/drive_letters.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/drive_letters.h`.
Canonical `make -n` compile/link (`build/test_drive_letters`):
```sh
cc -std=c11 -O2 -Wall -Wextra -Werror -Ispec -Ios/milton -Ibuild -DBLOCK_DEVICE_HOST -no-pie harness/diff/fat_diff/test_drive_letters.c os/milton/block_device.c -o build/test_drive_letters
```

### harness/diff/fat_diff/test_fat12_bpb.c — UNIT

`harness/diff/fat_diff/test_fat12_bpb.c:1`; - BPB parse / geometry oracle.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_fat12_bpb`.
Run/assert gate(s): `test-fat`, `test-fat12-bpb`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/blockdev_file.c`, `harness/diff/fat_diff/fat12_ref.py`, `harness/diff/fat_diff/fixtures/`, `harness/diff/fat_diff/test_fat12_bpb.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_bpb`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_bpb harness/diff/fat_diff/test_fat12_bpb.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_chain.c — UNIT

`harness/diff/fat_diff/test_fat12_chain.c:1`; - FAT12 decode + chain-walk oracle.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_fat12_chain`.
Run/assert gate(s): `test-fat`, `test-fat12-chain`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_ref.py`, `harness/diff/fat_diff/fixtures/`, `harness/diff/fat_diff/test_fat12_chain.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_chain`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_chain harness/diff/fat_diff/test_fat12_chain.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_dir.c — UNIT

`harness/diff/fat_diff/test_fat12_dir.c:1`; - FAT12 root-dir enumerate + read.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_fat12_dir`, `build/test_fat12_dir_mut_eoc`, `build/test_fat12_dir_mut_step`, `build/test_fat12_dir_mut_trunc`.
Run/assert gate(s): `test-fat`, `test-fat-readfile-mutant`, `test-fat12-dir`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_ref.py`, `harness/diff/fat_diff/fixtures/`, `harness/diff/fat_diff/test_fat12_dir.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_dir`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_dir harness/diff/fat_diff/test_fat12_dir.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_fault_rollback.c — UNIT

`harness/diff/fat_diff/test_fat12_fault_rollback.c:1`; - FAT12 WRITE-FAULT. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_fat12_fault_rollback`, `build/test_fat12_fault_rollback_mut_growdir`, `build/test_fat12_fault_rollback_mut_mkdir`, `build/test_fat12_fault_rollback_mut_writefile`.
Run/assert gate(s): `test-fat-fault-rollback`, `test-fat-fault-rollback-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/test_fat12_fault_rollback.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_fault_rollback`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_fault_rollback harness/diff/fat_diff/test_fat12_fault_rollback.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_mkdir.c — ORACLE-STRONG

`harness/diff/fat_diff/test_fat12_mkdir.c:1`; - FAT12 MKDIR differential vs mmd.. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_fat12_mkdir`, `build/test_fat12_mkdir_mut_dotdot`, `build/test_fat12_mkdir_mut_noempty`, `build/test_fat12_mkdir_mut_noeoc`, `build/test_fat12_mkdir_mutant_stale_trusted`.
Run/assert gate(s): `test-fat12-mkdir`, `test-fat12-mkdir-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/test_fat12_mkdir.c`, `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_mkdir`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_mkdir harness/diff/fat_diff/test_fat12_mkdir.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_mkdir_nested.c — ORACLE-STRONG

`harness/diff/fat_diff/test_fat12_mkdir_nested.c:1`; - FAT12 nested MKDIR/RMDIR. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_fat12_mkdir_nested`, `build/test_fat12_mkdir_nested_mut_nogrow`, `build/test_fat12_mkdir_nested_mut_norootmk`, `build/test_fat12_mkdir_nested_mut_norootrd`, `build/test_fat12_mkdir_nested_mut_rootscan`, `build/test_fat12_mkdir_nested_mut_rootslot`.
Run/assert gate(s): `test-m0bp`, `test-m0bp-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_ref.py`, `harness/diff/fat_diff/test_fat12_mkdir_nested.c`, `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_mkdir_nested`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_mkdir_nested harness/diff/fat_diff/test_fat12_mkdir_nested.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_mkdir_rollback.c — UNIT

`harness/diff/fat_diff/test_fat12_mkdir_rollback.c:1`; - FAT12 MKDIR post-grow. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_fat12_mkdir_rollback`, `build/test_fat12_mkdir_rollback_mut_nospace`.
Run/assert gate(s): `test-m0bp-rollback`, `test-m0bp-rollback-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/test_fat12_mkdir_rollback.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_mkdir_rollback`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_mkdir_rollback harness/diff/fat_diff/test_fat12_mkdir_rollback.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_move.c — ORACLE-STRONG

`harness/diff/fat_diff/test_fat12_move.c:1`; - FAT12 cross-directory MOVE oracle. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_fat12_move`, `build/test_fat12_move_mut_frees`, `build/test_fat12_move_mut_loses`, `build/test_fat12_move_mut_nodotdot`, `build/test_fat12_move_mut_noroll`, `build/test_fat12_move_mut_nosuffix`, `build/test_fat12_move_mut_relinks`.
Run/assert gate(s): `test-fat-move`, `test-fat-move-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_ref.py`, `harness/diff/fat_diff/fixtures/chain.txt`, `harness/diff/fat_diff/fixtures/hello.txt`, `harness/diff/fat_diff/fixtures/second.txt`, `harness/diff/fat_diff/test_fat12_move.c`, `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_move`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_move harness/diff/fat_diff/test_fat12_move.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_partial.c — UNIT

`harness/diff/fat_diff/test_fat12_partial.c:1`; - FAT12 positioned-read oracle.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_fat12_partial`, `build/test_fat12_partial_mut_pos`, `build/test_fat12_partial_mut_skip`.
Run/assert gate(s): `test-fat-partial`, `test-fat-partial-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_ref.py`, `harness/diff/fat_diff/fixtures/`, `harness/diff/fat_diff/test_fat12_partial.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_partial`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_partial harness/diff/fat_diff/test_fat12_partial.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_rename.c — ORACLE-STRONG

`harness/diff/fat_diff/test_fat12_rename.c:1`; - FAT12 RENAME differential vs mren.. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_fat12_rename`, `build/test_fat12_rename_mut_chain`, `build/test_fat12_rename_mut_dirstart`, `build/test_fat12_rename_mut_name8`, `build/test_fat12_rename_mut_nodest`.
Run/assert gate(s): `test-gnrc`, `test-gnrc-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fixtures/chain.txt`, `harness/diff/fat_diff/test_fat12_rename.c`, `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_rename`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_rename harness/diff/fat_diff/test_fat12_rename.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_subdir.c — UNIT

`harness/diff/fat_diff/test_fat12_subdir.c:1`; - FAT12 subdir + path traversal.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_fat12_subdir`, `build/test_fat12_subdir_mut_noattr`, `build/test_fat12_subdir_mut_single`.
Run/assert gate(s): `test-fat-subdir`, `test-fat-subdir-mutant`, `test-fat12-subdir`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_ref.py`, `harness/diff/fat_diff/fixtures/`, `harness/diff/fat_diff/test_fat12_subdir.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_subdir`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_subdir harness/diff/fat_diff/test_fat12_subdir.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_write.c — ORACLE-STRONG

`harness/diff/fat_diff/test_fat12_write.c:1`; - FAT12 WRITE unit + round-trip.. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_fat12_write`, `build/test_fat12_write_mut_eoc`, `build/test_fat12_write_mut_onefat`.
Run/assert gate(s): `test-fat-write`, `test-fat-write-mutant`, `test-fat12-write`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_ref.py`, `harness/diff/fat_diff/test_fat12_write.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_write`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_write harness/diff/fat_diff/test_fat12_write.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat12_write_partial.c — ORACLE-STRONG

`harness/diff/fat_diff/test_fat12_write_partial.c:1`; - FAT12 positioned-WRITE oracle.. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_fat12_write_partial`, `build/test_fat12_write_partial_mut_ext`, `build/test_fat12_write_partial_mut_rmw`.
Run/assert gate(s): `test-fat-write-partial`, `test-fat-write-partial-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_ref.py`, `harness/diff/fat_diff/test_fat12_write_partial.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat12_write_partial`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_write_partial harness/diff/fat_diff/test_fat12_write_partial.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat16.c — UNIT

`harness/diff/fat_diff/test_fat16.c:1`; - FAT16 read-path unit oracle (initech-z01).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_fat16`, `build/test_fat16_mut_m1`, `build/test_fat16_mut_m2`, `build/test_fat16_mut_m3`, `build/test_fat16_mut_m4`, `build/test_fat16_mut_m5`.
Run/assert gate(s): `test-fat16`, `test-fat16-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat16_ref.py`, `harness/diff/fat_diff/fixtures/`, `harness/diff/fat_diff/test_fat16.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat16`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat16 harness/diff/fat_diff/test_fat16.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat16_window.c — UNIT

`harness/diff/fat_diff/test_fat16_window.c:1`; - WINDOWED/streaming FAT-sector. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_fat16_window`, `build/test_fat16_window_mut_nowindow`, `build/test_fat16_window_mut_secoff`, `build/test_fat16_window_mut_straddle`.
Run/assert gate(s): `test-d27i`, `test-d27i-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fixtures/`, `harness/diff/fat_diff/test_fat16_window.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat16_window`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat16_window harness/diff/fat_diff/test_fat16_window.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_fat_spc.c — ORACLE-STRONG

`harness/diff/fat_diff/test_fat_spc.c:1`; FAT multi-sector-cluster differential driver (initech-fol7).. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_fat_spc`, `build/test_fat_spc_mut`.
Run/assert gate(s): `test-fat-spc`, `test-fat-spc-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat_spc.sh`, `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_fat_spc`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iharness/diff/fat_diff harness/diff/fat_diff/test_fat_spc.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c -o build/test_fat_spc
```

### harness/diff/fat_diff/test_partition.c — UNIT

`harness/diff/fat_diff/test_partition.c:1`; Factory mount/error oracle. Ref: IBM DOS 3.30 TR pp2-31,9-6..9;. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_partition`, `build/test_partition_EXTENDED`, `build/test_partition_IGNORE_BASE`, `build/test_partition_SIGNATURE`.
Run/assert gate(s): `test-block-device-emu`, `test-block-device-emu-mutant`, `test-fat-partition`, `test-fat-partition-emu`, `test-fat-partition-emu-mutant`, `test-fat-partition-mutant`, `test-partition-spec`, `test-partition-spec-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/block_device.sh`, `harness/diff/fat_diff/partition.sh`, `spec/mbr_partition_contract.json`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_partition`):
```sh
cc -std=c11 -O2 -Wall -Wextra -Werror -Ispec -Ios/milton -Iharness/diff/fat_diff harness/diff/fat_diff/test_partition.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c -o build/test_partition
```

### harness/proptest/test_memory_map.c — SIZE/LAYOUT

`harness/proptest/test_memory_map.c:1`; - host gate for the LOCKED physical. memory/layout, byte/size policy or legacy map constraints; preserve functional safety invariants separately.
Build target(s): `build/test_memory_map`.
Run/assert gate(s): `test-memory-map`, `test-memory-map-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/test_memory_map.c`, `spec/memory_map.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/memory_map.h`.
Canonical `make -n` compile/link (`build/test_memory_map`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec -Ios/milton -o build/test_memory_map harness/proptest/test_memory_map.c
```

### harness/proptest/test_panic_layout.c — SIZE/LAYOUT

`harness/proptest/test_panic_layout.c:1`; PRD Sec 5 / Appendix B; initech-s25 lane's two-line panel contract.. memory/layout, byte/size policy or legacy map constraints; preserve functional safety invariants separately.
Build target(s): `build/test_panic_layout`, `build/test_panic_layout_mut`.
Run/assert gate(s): `test-panic-layout`, `test-panic-layout-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_panic_layout`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ios/milton harness/proptest/test_panic_layout.c -o build/test_panic_layout
```

### harness/proptest/test_panic_reasons.c — UNIT

`harness/proptest/test_panic_reasons.c:1`; initech-480p/dbp1: literal causes from Rule 2, DEC-08a.5, DOS L030/DAS. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_panic_reasons`, `build/test_panic_reasons_mut`.
Run/assert gate(s): `test-panic-reason-map`, `test-panic-reason-map-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/toolbox_gate.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_panic_reasons`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ios/milton -Ispec harness/proptest/test_panic_reasons.c -o build/test_panic_reasons
```

### os/milton/test_40oq.c — UNIT

`os/milton/test_40oq.c:1`; CAPSTONE: INT 21h Appendix-A coverage certificate (beads initech-40oq).. deterministic assertions; mentions of properties/differentials include checks owned by separate suites, not sufficient to call this a strong oracle.
Build target(s): `build/test_40oq`, `build/test_40oq_mutant_getver`.
Run/assert gate(s): `test-40oq`, `test-40oq-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`, `spec/int21h_register.json`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_40oq`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_40oq os/milton/test_40oq.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/config_sys.c
```

### os/milton/test_ansi.c — UNIT

`os/milton/test_ansi.c:1`; host unit oracle for the ANSI.SYS escape-sequence FSM. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_ansi`, `build/test_ansi_mutant_noclamp`, `build/test_ansi_mutant_paramaccum`, `build/test_ansi_mutant_sgrcolor`, `build/test_ansi_mutant_wrapflip`.
Run/assert gate(s): `test-ansi`, `test-ansi-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_ansi`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/milton -Iseed -o build/test_ansi os/milton/test_ansi.c
```

### os/milton/test_ansi_bridge.c — UNIT

`os/milton/test_ansi_bridge.c:1`; initech-fzsvo: exercise the SAME adapter as kmain with the real console.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_ansi_bridge`, `build/test_ansi_bridge_mutant`.
Run/assert gate(s): `test-ansi-bridge`, `test-ansi-bridge-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_ansi_bridge`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ios/milton -Ios/flair -o build/test_ansi_bridge os/milton/test_ansi_bridge.c os/milton/ansi.c os/milton/console.c os/flair/surface.c
```

### os/milton/test_ansi_wire.c — UNIT

`os/milton/test_ansi_wire.c:1`; host oracle for WIRING the ANSI.SYS FSM into the CON. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_ansi_wire`, `build/test_ansi_wire_mutant_gate`.
Run/assert gate(s): `test-ansi-wire`, `test-ansi-wire-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_ansi_wire`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild -o build/test_ansi_wire os/milton/test_ansi_wire.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/config_sys.c
```

### os/milton/test_arena_disjoint.c — SIZE/LAYOUT

`os/milton/test_arena_disjoint.c:1`; host oracle for the AH=48h MCB arena being PROVABLY. memory/layout, byte/size policy or legacy map constraints; preserve functional safety invariants separately.
Build target(s): `build/test_arena_disjoint`, `build/test_arena_disjoint_mut`, `build/test_mapping_arena_disjoint`.
Run/assert gate(s): `test-arena-disjoint`, `test-arena-disjoint-mutant`, `test-host-mapping-arena_disjoint-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/initechmz_extended.h`, `spec/memory_map.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `os/milton/test_low_mapping.h`, `spec/initechmz_extended.h`.
Canonical `make -n` compile/link (`build/test_arena_disjoint`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild -o build/test_arena_disjoint os/milton/test_arena_disjoint.c os/milton/loader.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
```

### os/milton/test_ata.c — UNIT

`os/milton/test_ata.c:1`; Scripted ATA bus oracle. Ref: T13/1321D rev3 Sec 7.4 Command register,. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_ata`, `build/test_ata_errors`, `build/test_ata_flush`, `build/test_ata_lba`, `build/test_ata_prewait`.
Run/assert gate(s): `test-ata`, `test-ata-errors-mutant`, `test-ata-flush-mutant`, `test-ata-lba-mutant`, `test-ata-prewait-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_ata`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DATA_HOST_TEST -DATA_POLL_LIMIT=16 -Ispec -Ios/milton -Iseed -o build/test_ata os/milton/test_ata.c os/milton/ata.c
```

### os/milton/test_config_sys.c — UNIT

`os/milton/test_config_sys.c:1`; host unit oracle for the CONFIG.SYS parser (beads. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_config_sys`, `build/test_config_sys_mutant_off`, `build/test_config_sys_mutant_unk`.
Run/assert gate(s): `test-config-sys`, `test-config-sys-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_config_sys_baseline.txt`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_config_sys`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DCONFIG_SYS_BASELINE_PATH='"spec/dos_config_sys_baseline.txt"' -Ispec -Ios/milton -Iseed \
	-o build/test_config_sys os/milton/test_config_sys.c os/milton/config_sys.c
```

### os/milton/test_config_sys_fuzz.c — ORACLE-STRONG

`os/milton/test_config_sys_fuzz.c:1`; deterministic, seeded generative fuzzer for the PURE. property/fuzz invariants over generated inputs.
Build target(s): `build/test_config_sys_fuzz`, `build/test_config_sys_fuzz_mutant_noguard`, `build/test_config_sys_fuzz_mutant_offbyone`.
Run/assert gate(s): `test-config-fuzz`, `test-config-fuzz-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_fuzz.c`, `spec/dos_config_sys_baseline.txt`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_config_sys_fuzz`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed \
	-o build/test_config_sys_fuzz os/milton/test_config_sys_fuzz.c os/milton/config_sys.c
```

### os/milton/test_conin.c — UNIT

`os/milton/test_conin.c:1`; host unit oracle for the INT 21h CON INPUT functions. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_conin`, `build/test_conin_dual_ring`, `build/test_conin_mutant_count`, `build/test_conin_mutant_crwrap`, `build/test_conin_mutant_discard_ring_only`, `build/test_conin_mutant_nocooked`, `build/test_conin_mutant_noctrlc`, `build/test_conin_mutant_noecho`, `build/test_conin_mutant_template`, `build/test_conin_owner_pb`.
Run/assert gate(s): `test-4tw`, `test-4tw-mutant`, `test-conin-mutant`, `test-conin-unit`, `test-f3template-unit-mutant`, `test-input-owner-mutant`, `test-x8fs`, `test-x8fs-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_conin`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_conin os/milton/test_conin.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/kbd.c
```

### os/milton/test_console.c — UNIT

`os/milton/test_console.c:1`; - host blit-math oracle for the LFB text console.. deterministic assertions; mentions of properties/differentials include checks owned by separate suites, not sufficient to call this a strong oracle.
Build target(s): `build/test_console`, `build/test_console_cursor25_mutant`, `build/test_console_mutant_nowrap`.
Run/assert gate(s): `test-ansi-mutant`, `test-console`, `test-console-cursor25-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_console`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/milton -Iseed -Ios/flair \
	-o build/test_console os/milton/test_console.c os/milton/console.c os/flair/surface.c
```

### os/milton/test_devices.c — UNIT

`os/milton/test_devices.c:1`; host unit oracle for the DOS character-device chain. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_devices`, `build/test_devices_mutant_nodonebit`, `build/test_devices_mutant_nulreadbyte`.
Run/assert gate(s): `test-devices`, `test-devices-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_devices`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/milton -Iseed -o build/test_devices os/milton/test_devices.c
```

### os/milton/test_devwire.c — UNIT

`os/milton/test_devwire.c:1`; host unit oracle for the INT 21h <-> character-device-chain. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_devwire`, `build/test_devwire_mutant_nodevice`.
Run/assert gate(s): `test-devwire`, `test-devwire-mutant`, `test-j1lu-con-unit`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_devwire`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild -o build/test_devwire os/milton/test_devwire.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
```

### os/milton/test_dos_safety.c — UNIT

`os/milton/test_dos_safety.c:1`; Host integration of the real INT21/FAT stack. The thin shell grades disk. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_dos_protection_create`, `build/test_dos_protection_nonregular`, `build/test_dos_protection_open`, `build/test_dos_protection_unlink`, `build/test_dos_protection_write`, `build/test_dos_safety`, `build/test_dos_safety_mut_create`, `build/test_dos_safety_mut_identity`.
Run/assert gate(s): `test-dos-safety-create`, `test-dos-safety-create-mutant`, `test-dos-safety-create-ro`, `test-dos-safety-identity`, `test-dos-safety-identity-mutant`, `test-dos-safety-ro-mutant`, `test-dos-safety-unlink-ro`, `test-dos-safety-write-ro`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/dos_safety.sh`, `spec/dos_structs.h`, `spec/find_data.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`, `harness/diff/fat_diff/blockdev_file.h`, `spec/dos_structs.h`, `spec/find_data.h`.
Canonical `make -n` compile/link (`build/test_dos_protection_create`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DFAT12_MUTATE_CREATE_READONLY -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -Ibuild -o build/test_dos_protection_create os/milton/test_dos_safety.c os/milton/int21.c os/milton/devices.c os/milton/fileio_fat.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/sft.c os/milton/psp.c os/milton/mcb.c os/milton/irq.c
```

### os/milton/test_exec.c — UNIT

`os/milton/test_exec.c:1`; host unit oracle for INT 21h AH=4Bh EXEC + AH=4Dh GET-RETURN-. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_exec`, `build/test_exec_mutant_pathscan`, `build/test_exec_mutant_rc`.
Run/assert gate(s): `test-exec-mutant`, `test-exec-unit`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`, `spec/memory_map.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_exec`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_exec os/milton/test_exec.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
```

### os/milton/test_fileio.c — UNIT

`os/milton/test_fileio.c:1`; host unit oracle for the INT 21h file-handle functions. deterministic assertions; mentions of properties/differentials include checks owned by separate suites, not sufficient to call this a strong oracle.
Build target(s): `build/test_fileio`, `build/test_fileio_b53d_mut_getzero`, `build/test_fileio_b53d_mut_noalreject`, `build/test_fileio_b53d_mut_nocxreject`, `build/test_fileio_b53d_mut_nodispatch`, `build/test_fileio_mutant_chdir_noattr`, `build/test_fileio_mutant_gnrc_nodispatch`, `build/test_fileio_mutant_gnrc_nosamedir`, `build/test_fileio_mutant_gnrc_notfound`, `build/test_fileio_mutant_kji0_invert`, `build/test_fileio_mutant_kji0_nodispatch`, `build/test_fileio_mutant_kji0_noguard`, `build/test_fileio_mutant_kji0_wrongconst`, `build/test_fileio_mutant_lseek`, `build/test_fileio_mutant_nodrive`, `build/test_fileio_mutant_notroot`, `build/test_fileio_mutant_read`, `build/test_fileio_qekc_mut_getswap`, `build/test_fileio_qekc_mut_noalreject`, `build/test_fileio_qekc_mut_rook`.
Run/assert gate(s): `test-b53d-mutant`, `test-fileio`, `test-fileio-mutant`, `test-gnrc-int21-mutant`, `test-kji0-mutant`, `test-mzxa-mutant`, `test-qekc-mutant`, `test-u6wa-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_ref.py`, `harness/diff/fat_diff/fixtures/`, `spec/dos_structs.h`, `spec/find_data.h`, `spec/int21h_calling_convention.json`, `spec/int21h_register.json`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`, `spec/find_data.h`.
Canonical `make -n` compile/link (`build/test_fileio`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_fileio os/milton/test_fileio.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
```

### os/milton/test_fileio_subdir.c — UNIT

`os/milton/test_fileio_subdir.c:1`; - INT 21h subdir OPEN/READ over the REAL FAT12. deterministic assertions; mentions of properties/differentials include checks owned by separate suites, not sufficient to call this a strong oracle.
Build target(s): `build/test_fileio_subdir`, `build/test_fileio_subdir_b53d_mut_getdirreject`, `build/test_fileio_subdir_b53d_mut_noflush`, `build/test_fileio_subdir_b53d_mut_noreject`, `build/test_fileio_subdir_b53d_mut_notfound`, `build/test_fileio_subdir_mut_createroot`, `build/test_fileio_subdir_mut_grownoeoc`, `build/test_fileio_subdir_mut_grownoop`, `build/test_fileio_subdir_mut_noroot`, `build/test_fileio_subdir_mut_rootslot`, `build/test_fileio_subdir_mut_unlinknoop`, `build/test_fileio_subdir_nmpo_mut_noguard`, `build/test_fileio_subdir_qekc_mut_dropmdate`, `build/test_fileio_subdir_qekc_mut_noflush`, `build/test_fileio_subdir_qekc_mut_rootdir`.
Run/assert gate(s): `test-b53d`, `test-b53d-mutant`, `test-mzxa-integration`, `test-nmpo`, `test-nmpo-mutant`, `test-qekc`, `test-qekc-mutant`, `test-u6wa-mutant`, `test-zs24`, `test-zs24-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fat12_ref.py`, `harness/diff/fat_diff/fixtures/`, `spec/dos_structs.h`, `spec/find_data.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`, `harness/diff/fat_diff/blockdev_file.h`, `spec/dos_structs.h`, `spec/find_data.h`.
Canonical `make -n` compile/link (`build/test_fileio_subdir`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -Ibuild \
	-o build/test_fileio_subdir os/milton/test_fileio_subdir.c os/milton/int21.c os/milton/devices.c os/milton/fileio_fat.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/sft.c os/milton/psp.c os/milton/mcb.c os/milton/irq.c
```

### os/milton/test_fpu.c — UNIT

`os/milton/test_fpu.c:1`; - host oracle for the x87 bring-up DECISION logic. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_fpu_probe`, `build/test_fpu_probe_mut_absent_em`, `build/test_fpu_probe_mut_sw_only`.
Run/assert gate(s): `test-fpu-probe`, `test-fpu-probe-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/fpu_contract.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/fpu_contract.h`.
Canonical `make -n` compile/link (`build/test_fpu_probe`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Iseed -Ispec -Ios/milton -o build/test_fpu_probe os/milton/test_fpu.c
```

### os/milton/test_idt.c — UNIT

`os/milton/test_idt.c:1`; host unit oracle for the IDT gate encode + interrupt-frame. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_idt`, `build/test_idt_mutant`.
Run/assert gate(s): `test-idt`, `test-idt-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_idt`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/milton -Iseed \
	-o build/test_idt os/milton/test_idt.c os/milton/idt.c
```

### os/milton/test_int21.c — UNIT

`os/milton/test_int21.c:1`; host unit oracle for the INT 21h dispatch logic. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_int21`, `build/test_int21_mutant_4nbn_changeable`, `build/test_int21_mutant_4nbn_instatus`, `build/test_int21_mutant_4nbn_outstatus`, `build/test_int21_mutant_4nbn_setinfo`, `build/test_int21_mutant_dollar`, `build/test_int21_mutant_er3h_defoff`, `build/test_int21_mutant_er3h_dlraw`, `build/test_int21_mutant_er3h_nodisp`, `build/test_int21_mutant_er3h_setal`, `build/test_int21_mutant_noop`, `build/test_int21_mutant_ro6c_badhandle`, `build/test_int21_mutant_ro6c_conwrong`, `build/test_int21_mutant_ro6c_isdevinv`, `build/test_int21_mutant_ro6c_minorok`, `build/test_int21_mutant_ro6c_nodispatch`.
Run/assert gate(s): `test-4nbn-mutant`, `test-er3h`, `test-er3h-mutant`, `test-int21`, `test-int21-mutant`, `test-ro6c-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`, `spec/int21h_calling_convention.json`, `spec/int21h_register.json`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_int21`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_int21 os/milton/test_int21.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/config_sys.c
```

### os/milton/test_int21_edge.c — UNIT

`os/milton/test_int21_edge.c:1`; host unit oracle HARDENING the INT 21h error paths and. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_int21_edge`, `build/test_int21_edge_mutant_close`, `build/test_int21_edge_mutant_ptr`.
Run/assert gate(s): `test-int21-edge`, `test-int21-edge-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`, `spec/find_data.h`, `spec/int21h_calling_convention.json`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`, `spec/find_data.h`.
Canonical `make -n` compile/link (`build/test_int21_edge`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_int21_edge os/milton/test_int21_edge.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
```

### os/milton/test_int21_wildcard.c — UNIT

`os/milton/test_int21_wildcard.c:1`; host unit oracle for the DOS 8.3 FCB-style wildcard. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_int21_wildcard`, `build/test_int21_wildcard_mutant_bleed`, `build/test_int21_wildcard_mutant_exact`, `build/test_int21_wildcard_mutant_noup`, `build/test_int21_wildcard_mutant_qlit`.
Run/assert gate(s): `test-80k`, `test-80k-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_int21_wildcard`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DINT21_WILDCARD_TESTSEAM -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_int21_wildcard os/milton/test_int21_wildcard.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/config_sys.c
```

### os/milton/test_int24.c — UNIT

`os/milton/test_int24.c:1`; host unit oracle for the INT 22/23/24 handlers, SETVECT/. deterministic assertions; mentions of properties/differentials include checks owned by separate suites, not sufficient to call this a strong oracle.
Build target(s): `build/test_int24`, `build/test_int24_mutant_afswap`, `build/test_int24_mutant_getvect`, `build/test_int24_mutant_noreprompt`, `build/test_int24_mutant_vecoff`.
Run/assert gate(s): `test-int24`, `test-int24-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_messages.json`, `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_int24`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_int24 os/milton/test_int24.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
```

### os/milton/test_int24_wired.c — UNIT

`os/milton/test_int24_wired.c:1`; host oracle for the INT 24h critical-error WIRING. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_int24_wired`, `build/test_int24_wired_mut_noraise`, `build/test_int24_wired_mut_unbound`.
Run/assert gate(s): `test-int24-wired`, `test-int24-wired-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`, `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_int24_wired`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild -Iharness/diff/fat_diff \
	-o build/test_int24_wired os/milton/test_int24_wired.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/ata.c harness/diff/fat_diff/blockdev_file.c
```

### os/milton/test_kbd.c — ORACLE-STRONG

`os/milton/test_kbd.c:1`; host unit oracle for the PURE keyboard/PIT logic (beads. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/tbx_mut_KBD_MUT_SILENT_DROP`, `build/test_kbd`, `build/test_kbd_dual_ring`, `build/test_kbd_mutant_caps`, `build/test_kbd_mutant_ctrl_scan`, `build/test_kbd_mutant_extended`, `build/test_kbd_mutant_nav`, `build/test_kbd_mutant_ring`, `build/test_kbd_mutant_shift`, `build/test_kbd_mutant_table`.
Run/assert gate(s): `test-f3template-unit-mutant`, `test-input-owner-mutant`, `test-kbd-unit`, `test-kbd-unit-mutant`, `test-tbx-mutant-KBD_MUT_SILENT_DROP`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/expect_abort.h`, `spec/tsr_contract.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/expect_abort.h`.
Canonical `make -n` compile/link (`build/test_kbd`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Os -fno-strict-aliasing -fomit-frame-pointer -DKERNEL_PURE_DECODE_OZ -D_POSIX_C_SOURCE=200809L -Ios/milton -Iseed \
	-o build/test_kbd os/milton/test_kbd.c os/milton/kbd.c os/milton/pit.c
```

### os/milton/test_keep.c — UNIT

`os/milton/test_keep.c:1`; host oracle for INT 21h AH=31h KEEP (Terminate and Stay. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_keep`, `build/test_keep_mutant_noresident`, `build/test_mapping_keep`.
Run/assert gate(s): `test-host-mapping-keep-mutant`, `test-keep`, `test-keep-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `os/milton/test_low_mapping.h`, `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_keep`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_keep os/milton/test_keep.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
```

### os/milton/test_loader.c — UNIT

`os/milton/test_loader.c:1`; host unit oracle for the flat program loader's PREP. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_loader`, `build/test_loader_mutant_forceempty`, `build/test_loader_mutant_noguard`, `build/test_loader_mutant_nooffset`, `build/test_loader_mutant_noovlguard`, `build/test_loader_nested_mut`.
Run/assert gate(s): `test-loader`, `test-loader-mutant`, `test-loader-nested-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`, `spec/initechmz_extended.h`, `spec/memory_map.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`, `spec/initechmz_extended.h`, `spec/memory_map.h`.
Canonical `make -n` compile/link (`build/test_loader`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed \
	-o build/test_loader os/milton/test_loader.c os/milton/loader.c os/milton/psp.c
```

### os/milton/test_loader_big.c — UNIT

`os/milton/test_loader_big.c:1`; host unit oracle for the FAT IN-PLACE loader prep. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_loader_big`, `build/test_loader_big_mutant_stagingcap`.
Run/assert gate(s): `test-loader-big`, `test-loader-big-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/initechmz_extended.h`, `spec/memory_map.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/initechmz_extended.h`.
Canonical `make -n` compile/link (`build/test_loader_big`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -o build/test_loader_big os/milton/test_loader_big.c os/milton/loader.c os/milton/psp.c
```

### os/milton/test_mcb.c — ORACLE-STRONG

`os/milton/test_mcb.c:1`; host property suite for the MCB memory arena (initech-509.6).. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_mcb`, `build/test_mcb_mutant_coalesce`, `build/test_mcb_mutant_owner`.
Run/assert gate(s): `test-mcb`, `test-mcb-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_mcb`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed \
	-o build/test_mcb os/milton/test_mcb.c os/milton/mcb.c
```

### os/milton/test_mcb_int21.c — UNIT

`os/milton/test_mcb_int21.c:1`; host dispatch oracle for the INT 21h memory functions. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_mapping_mcb_int21`, `build/test_mcb_int21`, `build/test_mcb_int21_mutant_segbase`.
Run/assert gate(s): `test-host-mapping-mcb_int21-mutant`, `test-mcb-int21`, `test-mcb-int21-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `os/milton/test_low_mapping.h`, `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_mapping_mcb_int21`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DHOST_LOW_MUTATE_ZERO_OWNER -Ispec -Ios/milton -Iseed -Ibuild -o build/test_mapping_mcb_int21 os/milton/test_mcb_int21.c os/milton/loader.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
```

### os/milton/test_mz.c — UNIT

`os/milton/test_mz.c:1`; host unit oracle for the InitechMZ header-parse +. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_mz`, `build/test_mz_ext_length_mutant`, `build/test_mz_mutant_noadd`, `build/test_mz_mutant_paragraph`.
Run/assert gate(s): `test-mz`, `test-mz-extended-mutant`, `test-mz-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/initechmz_extended.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/initechmz_extended.h`.
Canonical `make -n` compile/link (`build/test_mz`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/milton -Iseed \
	-o build/test_mz os/milton/test_mz.c
```

### os/milton/test_mzload.c — UNIT

`os/milton/test_mzload.c:1`; host unit oracle for the InitechMZ (.EXE) loader integration. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_mzload`, `build/test_mzload_entry_equal`, `build/test_mzload_mutant_noreloc`.
Run/assert gate(s): `test-mzload`, `test-mzload-entry-emu`, `test-mzload-entry-emu-mutant`, `test-mzload-entry-mutant`, `test-mzload-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/mz_entry_emu.sh`, `spec/dos_structs.h`, `spec/initechmz_extended.h`, `spec/memory_map.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`, `spec/initechmz_extended.h`, `spec/memory_map.h`.
Canonical `make -n` compile/link (`build/test_mzload`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed \
	-o build/test_mzload os/milton/test_mzload.c os/milton/loader.c os/milton/psp.c
```

### os/milton/test_native.c — ORACLE-STRONG

`os/milton/test_native.c:1`; Independent DEC-03a.4 boundary/property vectors; byte snapshots on refusals. */. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_native`, `build/test_native_count16`, `build/test_native_owner`.
Run/assert gate(s): `test-native`, `test-native-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_native`):
```sh
cc -std=c11 -Wall -Wextra -Werror -o build/test_native os/milton/test_native.c
```

### os/milton/test_overlay.c — UNIT

`os/milton/test_overlay.c:1`; host oracle for INT 21h AH=4Bh AL=03h LOAD OVERLAY.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_overlay`, `build/test_overlay_mutant_chain`, `build/test_overlay_mutant_foreign`, `build/test_overlay_mutant_nofactor`, `build/test_overlay_mutant_obo`, `build/test_overlay_mutant_preflight`.
Run/assert gate(s): `test-overlay`, `test-overlay-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`, `spec/memory_map.h`, `spec/toolbox_gate.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`, `spec/memory_map.h`, `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_overlay`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_overlay os/milton/test_overlay.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/loader.c os/milton/fat12.c
```

### os/milton/test_psp.c — UNIT

`os/milton/test_psp.c:1`; host unit oracle for PSP construction (beads initech-509.4).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_psp`, `build/test_psp_mutant_int20`, `build/test_psp_mutant_jft`, `build/test_psp_mutant_tail`.
Run/assert gate(s): `test-psp`, `test-psp-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_psp`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed \
	-o build/test_psp os/milton/test_psp.c os/milton/psp.c
```

### os/milton/test_redir.c — UNIT

`os/milton/test_redir.c:1`; host unit oracle for STDOUT-handle redirection of the. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_redir`, `build/test_redir_mutant_noredir`.
Run/assert gate(s): `test-redir`, `test-redir-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_redir`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_redir os/milton/test_redir.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
```

### os/milton/test_rtc.c — UNIT

`os/milton/test_rtc.c:1`; host unit oracle for the PURE RTC conversion logic (beads. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_rtc`, `build/test_rtc_mutant_bcd`, `build/test_rtc_mutant_month`.
Run/assert gate(s): `test-rtc`, `test-rtc-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_rtc`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DRTC_HOST_TEST -Ispec -Ios/milton -Iseed \
	-o build/test_rtc os/milton/test_rtc.c os/milton/rtc.c
```

### os/milton/test_sft.c — UNIT

`os/milton/test_sft.c:1`; host unit oracle for the SFT/JFT handle layer (beads. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_sft`, `build/test_sft_mutant_dup`, `build/test_sft_mutant_dup2`, `build/test_sft_mutant_inherit`.
Run/assert gate(s): `test-sft`, `test-sft-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_sft`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed \
	-o build/test_sft os/milton/test_sft.c os/milton/sft.c os/milton/psp.c
```

### os/milton/test_tenant_load.c — UNIT

`os/milton/test_tenant_load.c:1`; host oracle for the DEC-08a PARAMETERIZED-BASE. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_tenant_load`, `build/test_tenant_load_mutant_progimage`.
Run/assert gate(s): `test-tenant-load`, `test-tenant-load-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`, `spec/memory_map.h`, `spec/toolbox_gate.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`, `spec/memory_map.h`, `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_tenant_load`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed \
	-o build/test_tenant_load os/milton/test_tenant_load.c os/milton/loader.c os/milton/psp.c
```

### os/milton/test_tsr.c — UNIT

`os/milton/test_tsr.c:1`; host oracle for the InitechDOS TSR kernel mechanics. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_tsr`.
Run/assert gate(s): `test-tsr-unit`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`, `spec/tsr_contract.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `os/milton/test_low_mapping.h`, `spec/dos_structs.h`, `spec/tsr_contract.h`.
Canonical `make -n` compile/link (`build/test_tsr`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_tsr os/milton/test_tsr.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
```

### os/milton/test_tsr_io.c — UNIT

`os/milton/test_tsr_io.c:1`; host oracle for the two device halves of the TSR contract. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_tsr_io`.
Run/assert gate(s): `test-tsr-io`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/tsr_contract.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/tsr_contract.h`.
Canonical `make -n` compile/link (`build/test_tsr_io`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Ios/flair -o build/test_tsr_io os/milton/test_tsr_io.c os/milton/kbd.c os/milton/console.c os/flair/surface.c
```

### tools/ppm_panic_check.c — UNIT

`tools/ppm_panic_check.c:1`; Independent full-frame oracle: PRD Appendix B + s25 lane panel layout.. structural/behavioral regression against authored geometry or implementation captures; no independent period/reference differential established for this checker.
Build target(s): `build/ppm_panic_check`.
Run/assert gate(s): `test-film-startup-screen`, `test-ovl-context`, `test-ovl-panic`, `test-panic-foreign-screen`, `test-panic-reasons-mutant`, `test-panic-screen`, `test-panic-screen-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/emu/ovl_panic.sh`, `harness/emu/panic_reasons.sh`, `harness/emu/panic_screen.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/ppm_panic_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror tools/ppm_panic_check.c -o build/ppm_panic_check
```

### tools/ppm_text_check.c — UNIT

`tools/ppm_text_check.c:1`; boot-to-text framebuffer oracle (factory, C-only).. structural/behavioral regression against authored geometry or implementation captures; no independent period/reference differential established for this checker.
Build target(s): `build/ppm_text_check`.
Run/assert gate(s): `test-boot`, `test-command-file-emu`, `test-command-file-emu-mutant`, `test-flair-exit-to-dos`, `test-flair-exit-to-dos-mutant`, `test-flair-i123-copy`, `test-flair-i123-hit`, `test-flair-i123-hit-mutant`, `test-flair-i123-keys`, `test-flair-i123-keys-mutant`, `test-flair-i123-menu`, `test-flair-i123-resize`, `test-flair-i123-resize-mutant`, `test-flair-i123-wide`, `test-flair-i123-wide-mutant`, `test-fs`, `test-samir-boot`, `test-samir-canon-salami`, `test-samir-canon-y2k`, `test-samir-console-emu`, `test-samir-console-emu-mutant`, `test-samir-list-layout-emu`, `test-samir-list-layout-emu-mutant`, `test-samir-numeric-format-emu`, `test-samir-numeric-format-emu-fraction-mutant`, `test-samir-numeric-format-emu-left-mutant`, `test-samir-report-emu`, `test-samir-status-emu`, `test-samir-status-emu-mutant`, `test-samir-status-off-emu`, `test-samir-write`, `test-shell`, `test-shell-dir-look`, `test-shell-dir-look-mutant`, `test-tracer-boot`, `test-w4-trace-smoke`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `fixtures/i123-copy/SINGLE.WK1`, `fixtures/i123-width//`, `harness/diff/dbf_diff/dbf_ref.py`, `harness/diff/dbf_diff/w4_trace.sh`, `harness/diff/dbf_diff/xbase_prog_diff/screen-mint/boot-list-real/SETUP/CLIENTS.DBF`, `harness/diff/dbf_diff/xbase_prog_diff/screen-mint/boot-list-real/list.screen`, `harness/diff/fat_diff/command_file.sh`, `harness/diff/fat_diff/shell_dir_look.sh`, `harness/emu/command_file.sh`, `harness/emu/qemu.c`, `spec/dos_banner.txt`, `spec/dos_messages.json`, `spec/i123/menu_level_visits.tsv`, `spec/i123/menu_tree.golden`.
External root(s): `DBASE3_DECOMP` → `../dbase3-decomp` (Makefile:39–43 defaults), `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Canonical `make -n` compile/link (`build/ppm_text_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -o build/ppm_text_check tools/ppm_text_check.c
```

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-kernel.md](gates-kernel.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**137 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 149 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-exec

#  recipe to execute (from 'Makefile', line 25824):

Boot/data images named by direct prerequisites or expanded recipe: `build/exec_boot.img`, `build/fat_exec.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@if grep -q 'triple_fault=1' "build/exec_boot.report"; then \
	printf '!!! test-exec FAIL: TRIPLE FAULT -- the FAT load or control transfer crashed\n'; \
	printf '!!! test-exec FAIL: no serial captured at %s\n' "build/exec_boot.serial"; exit 1; \
@grep -q '^LOADER-FAT-BIND-OK$$' "build/exec_boot.serial" \
	|| { printf '!!! test-exec FAIL: LOADER-FAT-BIND-OK missing -- the loader FAT volume never bound\n'; exit 1; }
@if grep -q '^EXEC-SAW-FAIL' "build/exec_boot.serial"; then \
	printf '!!! test-exec FAIL: EXEC-SAW-FAIL -- load_program_from_fat("GREET.COM") failed (root-cause the saw path, Rule 3):\n'; \
	grep '^EXEC-SAW-FAIL' "build/exec_boot.serial"; \
@grep -qF 'GREETINGS FROM A:GREET.COM' "build/exec_boot.serial" \
	|| { printf '!!! test-exec FAIL: "%s" missing -- the .COM loaded from FAT did not run/print via INT 21h\n' "GREETINGS FROM A:GREET.COM"; exit 1; }
@grep -q '^EXEC-SAW-EXIT rc=7$$' "build/exec_boot.serial" \
	|| { printf '!!! test-exec FAIL: EXEC-SAW-EXIT rc=7 missing -- the FROM-FAT program did not return rc=7 to the loader\n'; exit 1; }
```

### test-mzexec

#  recipe to execute (from 'Makefile', line 25693):

Boot/data images named by direct prerequisites or expanded recipe: `build/fat_mzexec.img`, `build/fat_mzforeign.img`, `build/mzexec_boot.img`, `build/mzforeign_boot.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@if grep -q 'triple_fault=1' "build/mzexec_run.report"; then \
	printf '!!! test-mzexec FAIL: TRIPLE FAULT booting/running the InitechMZ\n'; exit 1; fi
@grep -q 'marker_found=1' "build/mzexec_run.report" \
	|| { printf '!!! test-mzexec FAIL: MZEXEC-OK never appeared -- the .EXE did not run OR the reloc did not resolve\n'; cat "build/mzexec_run.report"; exit 1; }
@grep -q 'marker_found=1' "build/mzforeign_run.report" \
	|| { printf '!!! test-mzexec FAIL: foreign untagged MZ did NOT panic -- the DEC-08a.5 fail-loud gate is broken\n'; cat "build/mzforeign_run.report"; exit 1; }
```

### test-absdisk-emu

#  recipe to execute (from 'Makefile', line 26064):

Boot/data images named by direct prerequisites or expanded recipe: `build/absdisk_boot.img`, `build/absdisk_boot_data_run.img`, `build/fat_absdisk_data.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
	--expect "ABSDISK-EXIT rc=0" --quit-after "ABSDISK-EXIT rc=0" \
@grep -qxF '[harness] OK=1' "build/absdisk_boot.report"
@if grep -q 'triple_fault=1' "build/absdisk_boot.report"; then \
	printf '!!! test-absdisk-emu FAIL: TRIPLE FAULT -- root-cause the int25/26 entry->dispatch->IRETD path (Rule 3)\n'; exit 1; \
	printf '!!! test-absdisk-emu FAIL: no serial captured at %s\n' "build/absdisk_boot.serial"; exit 1; \
@tr -d '\r' < "build/absdisk_boot.serial" | grep -q '^ABSDISK-BIND-OK$$' \
	|| { printf '!!! test-absdisk-emu FAIL: ABSDISK-BIND-OK missing -- the absolute-disk seam never bound (no --disk2 / mount failed)\n'; exit 1; }
@if tr -d '\r' < "build/absdisk_boot.serial" | grep -q 'ABS-W26-FAIL\|ABS-R25-FAIL\|ABS-RT-FAIL'; then \
	printf '!!! test-absdisk-emu FAIL: an ABS-*-FAIL marker -- the int25/26 asm round-trip failed (root-cause, Rule 3):\n'; \
	tr -d '\r' < "build/absdisk_boot.serial" | grep 'ABS-.*-FAIL'; exit 1; \
@tr -d '\r' < "build/absdisk_boot.serial" | grep -q '^ABS-W26=OK$$' \
	|| { printf '!!! test-absdisk-emu FAIL: ABS-W26=OK missing -- int $$0x26 did not return CF=0 through the asm stub\n'; exit 1; }
```

### test-int21-dpb-emu

#  recipe to execute (from 'harness/diff/fat_diff/int21_dpb.mk', line 24):

Boot/data images named by direct prerequisites or expanded recipe: `build/tracer_secondary.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
```

### test-fpu

#  recipe to execute (from 'Makefile', line 27864):

Boot/data images named by direct prerequisites or expanded recipe: `build/fpu_test.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@grep -E '^(FPU|PANIC)' "build/fpu_test.serial" | tr -d '\r' | sed 's/^/    serial| /' || true
```

### test-fpu-absent

#  recipe to execute (from 'Makefile', line 27946):

Boot/data images named by direct prerequisites or expanded recipe: `build/fpu_absent.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@grep -E '^(FPU|PANIC|BANNER$$)' "build/fpu_absent.serial" | tr -d '\r' | sed 's/^/    serial| /' || true
```

### test-panic

#  recipe to execute (from 'Makefile', line 27764):

Boot/data images named by direct prerequisites or expanded recipe: `build/panic_boot.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
	printf '!!! test-panic FAIL: no serial captured at %s\n' "build/panic_boot.serial"; \
@grep -q '^SELFTEST-FAULT-ARMED$$' "build/panic_boot.serial" \
	|| { printf '!!! test-panic FAIL: never reached the deliberate fault (SELFTEST-FAULT-ARMED missing)\n'; exit 1; }
@grep -q 'PANIC vec=00' "build/panic_boot.serial" \
	|| { printf '!!! test-panic FAIL: no "PANIC vec=00" -- the #DE was not caught by the panic handler\n'; exit 1; }
@if grep -q 'triple_fault=1' "build/panic_boot.report"; then \
	printf '!!! test-panic FAIL: TRIPLE FAULT -- the exception cascaded to a silent reboot\n'; \
```

The remaining legacy emulator entries cover interrupts, x87/absent-FPU, DOS API edge/error/critical-error paths, drive mappings, FAT12/16 failures, EXEC/PSP/MCB/TSR lifetime and memory/placement regressions. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`os/milton/absdisk_program.asm:1`|0|0|none|
|`os/milton/ansi.c:1`|9|8|`os/milton/ansi.c:231` `ANSI_MUTATE_CONCEAL_BRIGHT`; `os/milton/ansi.c:244` `ANSI_MUTATE_SGR_COLOR`; `os/milton/ansi.c:257` `ANSI_MUTATE_SGR_COLOR`; `os/milton/ansi.c:307` `ANSI_MUTATE_ZERO_LITERAL`; `os/milton/ansi.c:315` `ANSI_MUTATE_STALE_REL`; `os/milton/ansi.c:339` `ANSI_MUTATE_NO_CLAMP`; `os/milton/ansi.c:439` `ANSI_MUTATE_WRAP_FLIP`; `os/milton/ansi.c:586` `ANSI_MUTATE_PARAM_ACCUM`; `os/milton/ansi.c:588` `ANSI_MUTATE_PARAM_UNBOUNDED`|
|`os/milton/ansi.h:1`|0|0|none|
|`os/milton/ansi_cursor.h:1`|1|1|`os/milton/ansi_cursor.h:19` `ANSI_MUTATE_BRIDGE_CLAMP`|
|`os/milton/ansi_probe_program.asm:1`|0|0|none|
|`os/milton/ata.c:1`|7|6|`os/milton/ata.c:36` `ATA_MUTATE_ERROR_COLLAPSE`; `os/milton/ata.c:135` `ATA_MUTATE_NO_PREWAIT`; `os/milton/ata.c:151` `ATA_MUTATE_LBA_WRAP`; `os/milton/ata.c:319` `ATA_MUTATE_NO_DRIVE_SUCCESS`; `os/milton/ata.c:428` `ATA_MUTATE_FLUSH_EARLY`; `os/milton/ata.c:549` `MVG_MUTATE_RETRY_UNBOUNDED`; `os/milton/ata.c:591` `MVG_MUTATE_RETRY_UNBOUNDED`|
|`os/milton/ata.h:1`|0|0|none|
|`os/milton/baked_exec_probe.asm:1`|0|0|none|
|`os/milton/block_device.c:1`|6|6|`os/milton/block_device.c:56` `DRIVE_MUT_DISK`; `os/milton/block_device.c:79` `DRIVE_MUT_RUNNING`; `os/milton/block_device.c:89` `BLOCK_DEVICE_MUT_HEADS`; `os/milton/block_device.c:108` `BLOCK_DEVICE_MUT_CARRY`; `os/milton/block_device.c:171` `BLOCK_DEVICE_MUT_RELATIVE`; `os/milton/block_device.c:177` `BLOCK_DEVICE_MUT_WATCH`|
|`os/milton/block_device.h:1`|0|0|none|
|`os/milton/block_device_probe.asm:1`|0|0|none|
|`os/milton/blockdev.h:1`|0|0|none|
|`os/milton/boot_info.h:1`|0|0|none|
|`os/milton/boot_screen.h:1`|1|1|`os/milton/boot_screen.h:174` `FLAIR_BOOT_MUT_NO_LOGO`|
|`os/milton/conedit.h:1`|8|8|`os/milton/conedit.h:40` `CONKEYS_MUT_F2`; `os/milton/conedit.h:45` `CONKEYS_MUT_F4`; `os/milton/conedit.h:56` `CONKEYS_MUT_INS`; `os/milton/conedit.h:61` `CONKEYS_MUT_DEL`; `os/milton/conedit.h:67` `CONKEYS_MUT_F1`; `os/milton/conedit.h:76` `CONKEYS_MUT_F3`; `os/milton/conedit.h:81` `CONKEYS_MUT_F5`; `os/milton/conedit.h:94` `CONKEYS_MUT_ESC`|
|`os/milton/config_sys.c:1`|5|4|`os/milton/config_sys.c:76` `CONFIG_MUTATE_NO_OVERFLOW_GUARD`; `os/milton/config_sys.c:81` `CONFIG_MUTATE_OVERFLOW_OFFBYONE`; `os/milton/config_sys.c:172` `CONFIG_MUTATE_FAIL_ON_UNKNOWN`; `os/milton/config_sys.c:210` `CONFIG_MUTATE_FILES_OFFBYONE`; `os/milton/config_sys.c:292` `CONFIG_MUTATE_FAIL_ON_UNKNOWN`|
|`os/milton/config_sys.h:1`|0|0|none|
|`os/milton/conin_program.asm:1`|0|0|none|
|`os/milton/conkeys_program.asm:1`|0|0|none|
|`os/milton/console.c:1`|5|5|`os/milton/console.c:104` `CONSOLE_MUTATE_CURSOR_OFF`; `os/milton/console.c:247` `CONSOLE_MUTATE_CLS_NO_HOME`; `os/milton/console.c:295` `CONSOLE_MUTATE_NOWRAP_IGNORED`; `os/milton/console.c:499` `CONSOLE_MUTATE_OFFGRID_CLAMP`; `os/milton/console.c:617` `CONSOLE_MUT_RESTORE_NO_DRAW`|
|`os/milton/console.h:1`|0|0|none|
|`os/milton/datetime_program.asm:1`|0|0|none|
|`os/milton/devices.c:1`|2|2|`os/milton/devices.c:141` `DEVICES_MUTATE_NUL_READ_BYTE`; `os/milton/devices.c:707` `DEVICES_MUTATE_NO_DONE_BIT`|
|`os/milton/devices.h:1`|0|0|none|
|`os/milton/dir_program.asm:1`|0|0|none|
|`os/milton/dospath.h:1`|0|0|none|
|`os/milton/exec_high_fixture.asm:1`|0|0|none|
|`os/milton/exec_high_low.asm:1`|0|0|none|
|`os/milton/execguard_program.asm:1`|0|0|none|
|`os/milton/exith_program.asm:1`|0|0|none|
|`os/milton/fat12.c:1`|77|69|`os/milton/fat12.c:30` `FAT_MUT_SPC_ONE`; `os/milton/fat12.c:65` `FAT12_MUTATE_PARTIAL_NO_RMW`; `os/milton/fat12.c:184` `FAT_PART_MUT_SIGNATURE`; `os/milton/fat12.c:202` `FAT_PART_MUT_EXTENDED`; `os/milton/fat12.c:216` `FAT_PART_MUT_IGNORE_BASE`; `os/milton/fat12.c:252` `FAT12_MUTATE_ACCEPT_BAD_BPB`; `os/milton/fat12.c:314` `FAT16_MUTATE_NO_CLASSIFY`; `os/milton/fat12.c:467` `FAT12_MUT_D27I_NO_STRADDLE`; `os/milton/fat12.c:489` `FAT12_MUT_D27I_FATSEC_OFFBYONE`; `os/milton/fat12.c:571` `FAT12_MUT_D27I_NO_WINDOW_DISPATCH`; `os/milton/fat12.c:580` `FAT16_MUTATE_ENTRY_MASK12`; `os/milton/fat12.c:607` `FAT16_MUTATE_ENTRY_OFFSET`; `os/milton/fat12.c:624` `FAT16_MUTATE_ENTRY_MASK12`; `os/milton/fat12.c:680` `FAT12_MUT_D27I_NO_WINDOW_DISPATCH`; `os/milton/fat12.c:748` `FAT16_MUTATE_EOC_THRESHOLD`; `os/milton/fat12.c:783` `FAT12_MUT_CHAIN_GUARD`; `os/milton/fat12.c:802` `FAT12_MUTATE_FILE_CHAIN_CHECK`; `os/milton/fat12.c:1192` `FAT12_MUTATE_READFILE_STEP_BOUND`; `os/milton/fat12.c:1227` `FAT12_MUTATE_READFILE_TRUNC`; `os/milton/fat12.c:1242` `FAT16_MUTATE_NO_CLUSTER2_BIAS`; `os/milton/fat12.c:1265` `FAT12_MUTATE_READFILE_EOC`; `os/milton/fat12.c:1404` `FAT12_MUTATE_PARTIAL_SKIP`; `os/milton/fat12.c:1413` `FAT12_MUTATE_PARTIAL_POS`; `os/milton/fat12.c:1453` `FAT12_MUTATE_NO_STEP_GUARD`; `os/milton/fat12.c:1513` `FAT12_MUTATE_NO_STEP_GUARD`; `os/milton/fat12.c:1596` `FAT12_MUTATE_ONE_FAT_COPY`; `os/milton/fat12.c:1643` `FAT12_MUTATE_CREATE_CHARS`; `os/milton/fat12.c:1774` `FAT12_MUTATE_GROW_NOOP`; `os/milton/fat12.c:1807` `FAT12_MUTATE_GROW_NO_WRAP`; `os/milton/fat12.c:1817` `FAT12_MUTATE_GROW_NO_EOC`; `os/milton/fat12.c:1837` `FAT12_MUTATE_GROWDIR_NO_ZEROFILL_ROLLBACK`; `os/milton/fat12.c:2044` `FAT12_MUTATE_SUBDIR_WRITE_ROOTSLOT`; `os/milton/fat12.c:2392` `FAT12_MUTATE_STALE_DIR_TRUSTED`; `os/milton/fat12.c:2459` `FAT12_MUTATE_STALE_DIR_TRUSTED`; `os/milton/fat12.c:2568` `FAT12_MUTATE_SETTIME_DROP_MDATE`; `os/milton/fat12.c:2578` `FAT12_MUTATE_SETTIME_NO_FLUSH`; `os/milton/fat12.c:2640` `FAT12_MUTATE_GETATTR_DIR_REJECT`; `os/milton/fat12.c:2693` `FAT12_MUTATE_SETATTR_NO_REJECT`; `os/milton/fat12.c:2714` `FAT12_MUTATE_SETATTR_NO_REJECT`; `os/milton/fat12.c:2723` `FAT12_MUTATE_SETATTR_NO_FLUSH`; `os/milton/fat12.c:2758` `FAT12_MUTATE_HIDE_DIR_NO_WRITE`; `os/milton/fat12.c:2815` `FAT12_MUTATE_CREATE_READONLY`; `os/milton/fat12.c:2823` `FAT12_MUTATE_CREATE_NONREGULAR`; `os/milton/fat12.c:2915` `FAT12_MUTATE_WRITE_READONLY`; `os/milton/fat12.c:2961` `FAT12_MUTATE_WRONG_EOC`; `os/milton/fat12.c:2986` `FAT12_MUTATE_WRITEFILE_NO_ROLLBACK`; `os/milton/fat12.c:3133` `FAT12_MUTATE_TRUNCATE_NO_FREE`; `os/milton/fat12.c:3224` `FAT12_MUTATE_WRITE_READONLY`; `os/milton/fat12.c:3278` `FAT12_MUTATE_PARTIAL_EXTEND_SHORT`; `os/milton/fat12.c:3346` `FAT12_MUTATE_PARTIAL_STALE_GAP`; `os/milton/fat12.c:3490` `FAT12_MUTATE_UNLINK_READONLY`; `os/milton/fat12.c:3493` `FAT12_MUTATE_UNLINK_NONREGULAR`; `os/milton/fat12.c:3554` `FAT12_MUTATE_RENAME_IGNORE_DIRSTART`; `os/milton/fat12.c:3591` `FAT12_MUTATE_RENAME_REJECT_DIR`; `os/milton/fat12.c:3601` `FAT12_MUTATE_RENAME_NO_DESTCHECK`; `os/milton/fat12.c:3632` `FAT12_MUTATE_RENAME_NAME_ONLY8`; `os/milton/fat12.c:3641` `FAT12_MUTATE_RENAME_TOUCH_CHAIN`; `os/milton/fat12.c:3933` `FAT12_MUTATE_MOVE_NO_DOTDOT_FIX`; `os/milton/fat12.c:3966` `FAT12_MUTATE_MOVE_RELINKS_CHAIN`; `os/milton/fat12.c:3974` `FAT12_MUTATE_MOVE_LOSES_ENTRY`; `os/milton/fat12.c:3990` `FAT12_MUTATE_MOVE_NO_GROW_ROLLBACK`; `os/milton/fat12.c:4024` `FAT12_MUTATE_MOVE_NO_GROW_ROLLBACK`; `os/milton/fat12.c:4031` `FAT12_MUTATE_MOVE_FREES_CHAIN`; `os/milton/fat12.c:4077` `FAT12_MUTATE_SUFFIX_NO_COLLIDE_CHECK`; `os/milton/fat12.c:4247` `FAT12_MUTATE_MKDIR_PARENT_ROOTONLY`; `os/milton/fat12.c:4258` `FAT12_MUTATE_MKDIR_PARENT_SCANROOT`; `os/milton/fat12.c:4284` `FAT12_MUTATE_MKDIR_PARENT_NOGROW`; `os/milton/fat12.c:4323` `FAT12_MUTATE_MKDIR_NO_NOSPACE_ROLLBACK`; `os/milton/fat12.c:4339` `FAT12_MUTATE_MKDIR_NO_EOC`; `os/milton/fat12.c:4371` `FAT12_MUTATE_MKDIR_NO_FLUSHFAIL_ROLLBACK`; `os/milton/fat12.c:4399` `FAT12_MUTATE_MKDIR_DOTDOT_SELF`; `os/milton/fat12.c:4450` `FAT12_MUTATE_MKDIR_OWNENTRY_ROOTSLOT`; `os/milton/fat12.c:4483` `FAT12_MUTATE_RMDIR_NO_EMPTYCHECK`; `os/milton/fat12.c:4531` `FAT12_MUTATE_RMDIR_PARENT_ROOTONLY`; `os/milton/fat12.c:4563` `FAT12_MUTATE_RMDIR_NO_EMPTYCHECK`; `os/milton/fat12.c:4708` `FAT12_MUTATE_SUBDIR_SINGLESECTOR`; `os/milton/fat12.c:4939` `FAT12_MUTATE_SUBDIR_NOATTR`|
|`os/milton/fat12.h:1`|0|0|none|
|`os/milton/fcbx_program.asm:1`|0|0|none|
|`os/milton/fdisk_fixture.asm:1`|0|0|none|
|`os/milton/fileio_fat.c:1`|14|14|`os/milton/fileio_fat.c:76` `DOS_VOLUMES_MUT_CACHE`; `os/milton/fileio_fat.c:85` `DOS_VOLUMES_MUT_INVALID`; `os/milton/fileio_fat.c:90` `DOS_VOLUMES_MUT_PATH`; `os/milton/fileio_fat.c:111` `FILEIO_MUTATE_NO_STAMP`; `os/milton/fileio_fat.c:184` `DOS_VOLUMES_MUT_FAT16`; `os/milton/fileio_fat.c:282` `FILEIO_MUTATE_SUBDIR_CREATE_ROOTONLY`; `os/milton/fileio_fat.c:330` `SAMIR_MUTATE_FULL_ERROR`; `os/milton/fileio_fat.c:375` `FILEIO_MUTATE_SETTIME_ROOTDIR`; `os/milton/fileio_fat.c:425` `FILEIO_MUTATE_CHMOD_NOTFOUND_ACCESS`; `os/milton/fileio_fat.c:449` `FILEIO_MUTATE_SUBDIR_UNLINK_NOOP`; `os/milton/fileio_fat.c:624` `FILEIO_MUTATE_DIR_FREE`; `os/milton/fileio_fat.c:634` `FILEIO_MUTATE_DIR_BOOT_LABEL`; `os/milton/fileio_fat.c:658` `DU_MUTATE_LABEL_CACHE`; `os/milton/fileio_fat.c:707` `FILEIO_MUTATE_CWD_NOROOT`|
|`os/milton/fileio_fat.h:1`|0|0|none|
|`os/milton/fpu.h:1`|2|2|`os/milton/fpu.h:60` `FPU_MUTATE_PROBE_SW_ONLY`; `os/milton/fpu.h:82` `FPU_MUTATE_ABSENT_KEEPS_EM_CLEAR`|
|`os/milton/gobble_program.asm:1`|0|0|none|
|`os/milton/greet_program.asm:1`|0|0|none|
|`os/milton/hygiene_probe.asm:1`|0|0|none|
|`os/milton/idt.c:1`|1|1|`os/milton/idt.c:49` `IDT_MUTATE_OFFSET_HI`|
|`os/milton/idt.h:1`|0|0|none|
|`os/milton/int21.c:1`|102|100|`os/milton/int21.c:135` `DOS_VOLUMES_MUT_LASTDRIVE`; `os/milton/int21.c:149` `DOS_VOLUMES_MUT_CWD`; `os/milton/int21.c:347` `INT21_MUTATE_BREAK_DEFAULT_OFF`; `os/milton/int21.c:667` `INT21_MUT_SETVECT_TRAP`; `os/milton/int21.c:682` `INT21_MUT_KEEP_INPUT_LOOKAHEAD`; `os/milton/int21.c:802` `ANSIWIRE_MUTATE_IGNORE_GATE`; `os/milton/int21.c:844` `ANSIWIRE_MUTATE_DROP_REL`; `os/milton/int21.c:900` `ANSIWIRE_MUTATE_DROP_DSR`; `os/milton/int21.c:941` `CONKEYS_MUT_PRN`; `os/milton/int21.c:1159` `DOS_VOLUMES_MUT_HANDLE`; `os/milton/int21.c:1213` `K36G_MUTATE_NO_REDIR`; `os/milton/int21.c:1282` `INT21_MUTATE_PUTS_EMIT_DOLLAR`; `os/milton/int21.c:1346` `CONKEYS_MUT_PAUSE`; `os/milton/int21.c:1379` `INT21_MUTATE_NO_CTRLC_CHECK`; `os/milton/int21.c:1435` `INT21_MUTATE_CONIN_NO_ECHO`; `os/milton/int21.c:1550` `INT21_MUTATE_CARET_Z`; `os/milton/int21.c:1605` `CMD_MUT_NO_TEMPLATE`; `os/milton/int21.c:1626` `INT21_MUTATE_BUFINPUT_CR_WRAP`; `os/milton/int21.c:1636` `INT21_MUTATE_BUFINPUT_COUNT_CR`; `os/milton/int21.c:1655` `INT21_MUTATE_DISCARD_RING_ONLY`; `os/milton/int21.c:1770` `INT21_MUTATE_NO_PTR_GUARD`; `os/milton/int21.c:1910` `INT21_MUTATE_IOCTL_BADHANDLE_OK`; `os/milton/int21.c:1924` `INT21_MUTATE_IOCTL_CON_WRONG`; `os/milton/int21.c:1931` `INT21_MUTATE_IOCTL_ISDEV_INVERT`; `os/milton/int21.c:1970` `INT21_MUTATE_IOCTL_SETINFO_NOGUARD`; `os/milton/int21.c:2005` `INT21_MUTATE_IOCTL_INSTATUS_EOF_FLIP`; `os/milton/int21.c:2030` `INT21_MUTATE_IOCTL_OUTSTATUS_NOTREADY`; `os/milton/int21.c:2056` `INT21_MUTATE_IOCTL_CHANGEABLE_OK`; `os/milton/int21.c:2071` `INT21_MUTATE_IOCTL_MINOR_OK`; `os/milton/int21.c:2109` `INT21_MUTATE_PATHSCAN_NOBOUND`; `os/milton/int21.c:2133` `INT21_MUTATE_PATHSCAN_NOBOUND`; `os/milton/int21.c:2170` `INT21_MUTATE_RESOLVE_NODRIVE`; `os/milton/int21.c:2193` `INT21_MUTATE_RESOLVE_NOTROOT`; `os/milton/int21.c:2416` `INT21_MUTATE_OPEN_NO_DEVICE`; `os/milton/int21.c:2509` `INT21_MUTATE_SAMEFILE_FALSE`; `os/milton/int21.c:2647` `INT21_MUTATE_OPEN_READONLY`; `os/milton/int21.c:2812` `INT21_MUTATE_CREATNEW_NO_GUARD`; `os/milton/int21.c:2842` `INT21_MUTATE_CREATNEW_GUARD_INVERT`; `os/milton/int21.c:2847` `INT21_MUTATE_CREATNEW_WRONG_CONST`; `os/milton/int21.c:2997` `INT21_MUTATE_RENAME_NO_SAMEDIR_GUARD`; `os/milton/int21.c:3021` `INT21_MUTATE_RENAME_NOTFOUND_PATH`; `os/milton/int21.c:3071` `INT21_MUTATE_CHMOD_NO_AL_REJECT`; `os/milton/int21.c:3115` `INT21_MUTATE_CHMOD_NO_CX_REJECT`; `os/milton/int21.c:3141` `INT21_MUTATE_CHMOD_GET_ZERO`; `os/milton/int21.c:3169` `INT21_MUTATE_MKDIR_DEVICE`; `os/milton/int21.c:3344` `INT21_MUTATE_CON_EOF_IGNORED`; `os/milton/int21.c:3385` `INT21_MUTATE_CONHANDLE_NOCOOKED`; `os/milton/int21.c:3390` `INT21_MUTATE_CON_EXPLICIT_RAW`; `os/milton/int21.c:3455` `DOS_VOLUMES_MUT_HANDLE`; `os/milton/int21.c:3466` `INT21_MUTATE_READ_IGNORE_OFFSET`; `os/milton/int21.c:3513` `INT21_MUTATE_CLOSE_NO_REFGUARD`; `os/milton/int21.c:3570` `INT21_MUTATE_LSEEK_WHENCE`; `os/milton/int21.c:3613` `INT21_MUTATE_FILETIME_NO_AL_REJECT`; `os/milton/int21.c:3650` `INT21_MUTATE_FILETIME_GET_SWAP`; `os/milton/int21.c:3671` `INT21_MUTATE_FILETIME_RO_OK`; `os/milton/int21.c:3758` `INT21_MUTATE_WILD_STAR_BLEED`; `os/milton/int21.c:3775` `INT21_MUTATE_WILD_NO_UPCASE`; `os/milton/int21.c:3810` `INT21_MUTATE_WILD_MATCH_EXACT`, `INT21_MUTATE_WILD_QMARK_LITERAL`; `os/milton/int21.c:3885` `DU_MUT_LABEL`; `os/milton/int21.c:3913` `DOS_VOLUMES_MUT_SEARCH`; `os/milton/int21.c:4092` `INT21_MUTATE_DPB_FIRST_DATA`; `os/milton/int21.c:4269` `INT21_MUTATE_FCB_PARTIAL_NOZERO`; `os/milton/int21.c:4327` `INT21_MUTATE_FCB_EXT_NOATTR`; `os/milton/int21.c:4397` `INT21_MUTATE_FCB_SEQ_NO_ADVANCE`; `os/milton/int21.c:4516` `INT21_MUTATE_OVL_BOUNDS_OBO`; `os/milton/int21.c:4576` `INT21_MUTATE_EXEC_ROOTREJECT`; `os/milton/int21.c:4688` `INT21_MUT_INDOS_EXEC`; `os/milton/int21.c:4717` `INT21_MUTATE_RETCODE_ZERO`; `os/milton/int21.c:4815` `INT21_MUTATE_ALLOC_NO_SEGBASE`; `os/milton/int21.c:5004` `TSR_MUT_NO_KEEP`; `os/milton/int21.c:5048` `DOS_VOLUMES_MUT_SELECT`; `os/milton/int21.c:5052` `DOS_VOLUMES_MUT_LIMIT`; `os/milton/int21.c:5219` `INT21_MUTATE_CD_ROOT`; `os/milton/int21.c:5229` `INT21_MUTATE_CD_ROOT`; `os/milton/int21.c:5380` `GETVECT_MUTATE_AX`; `os/milton/int21.c:5423` `INT21_MUTATE_BREAK_DL_RAW`; `os/milton/int21.c:5432` `INT21_MUTATE_BREAK_SET_WRITES_AL`; `os/milton/int21.c:5488` `CRIT_MUTATE_AF_SWAP`; `os/milton/int21.c:5521` `INT21_MUTATE_CARET_C`; `os/milton/int21.c:5530` `INT21_MUTATE_BREAK_RECOVERY`; `os/milton/int21.c:5565` `INT24_MUTATE_NO_REPROMPT`; `os/milton/int21.c:5592` `INT21_MUTATE_DISK_BREAK`; `os/milton/int21.c:5594` `INT21_MUTATE_BREAK_CLEANUP`; `os/milton/int21.c:5678` `CERT_MUTATE_DROP_GETVER`; `os/milton/int21.c:5691` `INT21_MUTATE_BREAK_NO_DISPATCH`; `os/milton/int21.c:5707` `INT21_MUTATE_CREATNEW_NO_DISPATCH`; `os/milton/int21.c:5739` `INT21_MUTATE_RENAME_NO_DISPATCH`; `os/milton/int21.c:5751` `INT21_MUTATE_CHMOD_NO_DISPATCH`; `os/milton/int21.c:5767` `INT21_MUTATE_FILETIME_NO_DISPATCH`; `os/milton/int21.c:5778` `INT21_MUTATE_IOCTL_NO_DISPATCH`; `os/milton/int21.c:5874` `INT21_MUTATE_UNLISTED_NOOP`; `os/milton/int21.c:6021` `INT21_MUTATE_NOT_READY`; `os/milton/int21.c:6043` `MVG_MUTATE_NO_RAISE`; `os/milton/int21.c:6087` `ABSDISK_MUTATE_WRONG_ERRCLASS`; `os/milton/int21.c:6119` `ABSDISK_MUTATE_REG_TRANSPOSE`; `os/milton/int21.c:6136` `ABSDISK_MUTATE_OFF_BY_ONE_LBA`; `os/milton/int21.c:6153` `ABSDISK_MUTATE_PACKET_LITERAL`; `os/milton/int21.c:6167` `ABSDISK_MUTATE_DRIVE_A_ONLY`; `os/milton/int21.c:6204` `ABSDISK_MUTATE_NO_BOUNDS`; `os/milton/int21.c:6228` `ABSDISK_MUTATE_WRITE_NOOP`; `os/milton/int21.c:6233` `ABSDISK_MUTATE_WRITE_NEIGHBOR`; `os/milton/int21.c:6263` `ABSDISK_MUTATE_STACK_WART`|
|`os/milton/int21.h:1`|0|0|none|
|`os/milton/io.h:1`|0|0|none|
|`os/milton/irq.c:1`|0|0|none|
|`os/milton/irq.h:1`|0|0|none|
|`os/milton/irqstorm_program.asm:1`|0|0|none|
|`os/milton/isr.asm:1`|0|0|none|
|`os/milton/kbd.c:1`|13|13|`os/milton/kbd.c:55` `KBD_MUTATE_RING_OFFBYONE`; `os/milton/kbd.c:80` `KBD_MUT_SILENT_DROP`; `os/milton/kbd.c:163` `KBD_MUTATE_E0`; `os/milton/kbd.c:175` `KBD_MUTATE_CTRL_RELEASE_ALL`; `os/milton/kbd.c:189` `KBD_MUT_NO_ALT`; `os/milton/kbd.c:212` `KBD_MUT_ONE_SHIFT`; `os/milton/kbd.c:222` `KBD_MUT_CAPS_REPEAT`; `os/milton/kbd.c:244` `KBD_MUTATE_SCANCODE_TABLE`; `os/milton/kbd.c:257` `KBD_MUTATE_NO_CTRL`; `os/milton/kbd.c:322` `KBD_MUT_DROP_NAV`; `os/milton/kbd.c:325` `KBD_MUT_WRONG_CTRL_SCAN`; `os/milton/kbd.c:329` `KBD_MUT_DROP_EXTENDED`; `os/milton/kbd.c:365` `KBD_MUT_DUAL_RING`|
|`os/milton/kbd.h:1`|0|0|none|
|`os/milton/kbd_echo_program.asm:1`|0|0|none|
|`os/milton/kernel.ld:1`|0|0|none|
|`os/milton/kmain.c:1`|54|47|`os/milton/kmain.c:483` `TSR_MUT_NO_IDLE`; `os/milton/kmain.c:605` `CHKDSK_MUT_CACHE`; `os/milton/kmain.c:887` `KMAIN_MUT_EXEC_NO_COMSPEC`; `os/milton/kmain.c:1155` `FLAIR_DAC_MUT_6BIT`; `os/milton/kmain.c:1159` `FLAIR_DAC_MUT_OMIT119`; `os/milton/kmain.c:1162` `FLAIR_DAC_MUT_OMIT165`; `os/milton/kmain.c:1171` `FLAIR_DAC_MUT_CHANNEL`; `os/milton/kmain.c:1250` `FLAIR_DAC_MUT_ROWCOPY`; `os/milton/kmain.c:1756` `FLAIR_LIVE_MUTATE_NO_ROUTE_ON_CHROME`; `os/milton/kmain.c:1840` `FLAIR_LIVE_MUTATE_DROP_UPDATE`; `os/milton/kmain.c:1852` `FLAIR_LIVE_MUTATE_NO_MENUBAR_SWAP`; `os/milton/kmain.c:1873` `FLAIR_LIVE_MUTATE_NO_DRAG_CLAMP`; `os/milton/kmain.c:2019` `FLAIR_LIVE_MUTATE_NO_DRAG_CLAMP`; `os/milton/kmain.c:2055` `KMAIN_MUT_TRACK_TIMEOUT`; `os/milton/kmain.c:2066` `KMAIN_MUT_TRACK_TIMEOUT`; `os/milton/kmain.c:2145` `FLAIR_LIVE_MUTATE_DRAG_NOOP`; `os/milton/kmain.c:2237` `FLAIR_LIVE_MUTATE_DRAG_NOOP`; `os/milton/kmain.c:2241` `FLAIR_LIVE_MUTATE_MODAL_NO_RESTORE`; `os/milton/kmain.c:2309` `KMAIN_MUT_GOAWAY_ON_PRESS`; `os/milton/kmain.c:2542` `KMAIN_MUT_CLOSE_NO_ASK`; `os/milton/kmain.c:2555` `KMAIN_MUT_TENANT_CLOSE_HIDE`; `os/milton/kmain.c:2751` `FLAIR_LIVE_MUTATE_MENU_NOOP`; `os/milton/kmain.c:2775` `KMAIN_MUT_MENU_RESTORE_SHELLRENDER`; `os/milton/kmain.c:2857` `FLAIR_LIVE_MUTATE_MENU_NOOP`; `os/milton/kmain.c:2916` `KMAIN_MUT_MENU_NO_REHIT`; `os/milton/kmain.c:2957` `KMAIN_MUT_MENU_NO_RESTORE`; `os/milton/kmain.c:3083` `KMAIN_MUT_NO_INPUT_REPLAY`; `os/milton/kmain.c:3135` `FLAIR_LIVE_MUTATE_NO_REBUILD`; `os/milton/kmain.c:3645` `KMAIN_MUT_OVL_UNBOUND`; `os/milton/kmain.c:4431` `KMAIN_MUT_ALERT_NODRAW`; `os/milton/kmain.c:4997` `SF_MUT_BADNAME_SILENT`; `os/milton/kmain.c:5216` `KMAIN_MUT_TRACK_TIMEOUT`; `os/milton/kmain.c:5236` `KMAIN_MUT_COMMAND_VOLUME`, `OVL_MUT_DATA_VOLUME`; `os/milton/kmain.c:5248` `KMAIN_MUT_COMMAND_VOLUME`, `OVL_MUT_DATA_VOLUME`; `os/milton/kmain.c:5342` `OVL_MUT_NO_IRQ_GUARD`; `os/milton/kmain.c:5345` `OVL_MUT_NO_PANIC_GUARD`; `os/milton/kmain.c:5348` `OVL_MUT_NO_BUSY_GUARD`; `os/milton/kmain.c:5352` `OVL_MUT_DATA_VOLUME`; `os/milton/kmain.c:5355` `OVL_MUT_NO_MISSING`; `os/milton/kmain.c:5369` `OVL_MUT_NO_MAGIC`; `os/milton/kmain.c:5382` `OVL_MUT_NO_IMPORTS`; `os/milton/kmain.c:5390` `OVL_MUT_NO_CRC`; `os/milton/kmain.c:5448` `OVL_MUT_NO_FAULT_NAME`; `os/milton/kmain.c:5498` `KMAIN_MUT_COMMAND_VOLUME`; `os/milton/kmain.c:5587` `KMAIN_MUT_FG_FOLLOWS_FRONT`; `os/milton/kmain.c:5640` `KMAIN_MUT_NO_DESK_ACTIVATE`; `os/milton/kmain.c:6554` `KMAIN_MUT_NOTES_BAR_SYS`; `os/milton/kmain.c:6632` `CURSOR_MUT_HIDE_AT_BOOT`; `os/milton/kmain.c:6660` `FLAIR_LIVE_MUTATE_NO_HOOK`; `os/milton/kmain.c:6674` `FLAIR_LIVE_MUTATE_NO_KBD_HOOK`; `os/milton/kmain.c:6696` `FLAIR_LIVE_MUTATE_NO_MOUSE_HOOK`; `os/milton/kmain.c:6851` `KMAIN_MUT_CP_NO_SYSTEM_BAR`; `os/milton/kmain.c:7054` `KMAIN_MUT_MENU2_DEAD`; `os/milton/kmain.c:7067` `KMAIN_MUT_MENU2_BAR_SYS`|
|`os/milton/kstart.asm:1`|0|0|none|
|`os/milton/loader.c:1`|12|12|`os/milton/loader.c:68` `LOADER_MUT_NO_RESTORE`; `os/milton/loader.c:114` `LOADER_MUTATE_FORCE_EMPTY_ENV`; `os/milton/loader.c:208` `LOADER_MUTATE_TENANT_PROGRAM_IMAGE_BASE`; `os/milton/loader.c:299` `LOADER_MUTATE_OVL_ACCEPT_FOREIGN`; `os/milton/loader.c:312` `LOADER_MUTATE_OVL_COPY_FIRST`; `os/milton/loader.c:628` `LOADER_MUTATE_INPLACE_DOUBLE_HANDLE`; `os/milton/loader.c:774` `LOADER_MUT_CWD_RESET`; `os/milton/loader.c:1049` `LOADER_MUTATE_EXEC_ROOTONLY`; `os/milton/loader.c:1098` `LOADER_MUT_HIGH_OWNER`; `os/milton/loader.c:1147` `LOADER_MUT_TSR_EXEC_OVERLAP`; `os/milton/loader.c:1213` `LOADER_MUT_TSR_EXEC_ALLOW_FLAT`; `os/milton/loader.c:1377` `LOADER_MUT_HIGH_RECLAIM`|
|`os/milton/loader.h:1`|0|0|none|
|`os/milton/loader_cold.c:1`|11|10|`os/milton/loader_cold.c:39` `LOADER_MUT_NO_OVERLAY_GUARD`; `os/milton/loader_cold.c:53` `LOADER_MUT_NO_TRANSIENT_GUARD`; `os/milton/loader_cold.c:95` `LOADER_MUTATE_REIMPOSE_STAGING_CAP`; `os/milton/loader_cold.c:120` `LOADER_MUTATE_NO_OFFSET`; `os/milton/loader_cold.c:160` `LOADER_MUTATE_ARENA_OVERLAP`; `os/milton/loader_cold.c:274` `LOADER_MUT_HIGH_LOW`; `os/milton/loader_cold.c:371` `LOADER_MUTATE_MZ_NO_RELOC`; `os/milton/loader_cold.c:383` `LOADER_MUT_TSR_EXEC_FIXED_RELOC`; `os/milton/loader_cold.c:427` `LOADER_MUTATE_MZ_ENTRY_EQUAL`; `os/milton/loader_cold.c:542` `LOADER_MUTATE_OVL_NO_FACTOR`; `os/milton/loader_cold.c:560` `LOADER_MUTATE_OVL_NO_FACTOR`|
|`os/milton/loader_cold.h:1`|0|0|none|
|`os/milton/mcb.c:1`|3|3|`os/milton/mcb.c:150` `MCB_MUTATE_NO_COALESCE`; `os/milton/mcb.c:272` `MCB_MUTATE_NO_OWNER_CHECK`; `os/milton/mcb.c:391` `MCB_MUTATE_KEEP_NO_RESIDENT`|
|`os/milton/mcb.h:1`|0|0|none|
|`os/milton/mouse.c:1`|1|1|`os/milton/mouse.c:252` `FLAIR_LIVE_MUTATE_MASTER_ONLY_EOI`|
|`os/milton/mouse.h:1`|0|0|none|
|`os/milton/mouse_pack.h:1`|3|3|`os/milton/mouse_pack.h:60` `MOUSE_MUT_SIGN_BYTE`; `os/milton/mouse_pack.h:71` `MOUSE_MUT_EARLY_BUTTON`; `os/milton/mouse_pack.h:107` `EVENT_MUTATE_FLIP_SIGN`|
|`os/milton/multiopen_program.asm:1`|0|0|none|
|`os/milton/mz.c:1`|4|4|`os/milton/mz.c:121` `MZ_MUTATE_ACCEPT_FOREIGN`; `os/milton/mz.c:198` `MZ_MUT_EXT_LENGTH`; `os/milton/mz.c:258` `MZ_MUTATE_RELOC_NOADD`; `os/milton/mz.c:264` `MZ_MUTATE_RELOC_PARAGRAPH`|
|`os/milton/mz.h:1`|0|0|none|
|`os/milton/mzexec_fixture.asm:1`|0|0|none|
|`os/milton/native.c:1`|2|2|`os/milton/native.c:47` `NATIVE_MUT_COUNT16`; `os/milton/native.c:50` `NATIVE_MUT_OWNER`|
|`os/milton/nested_exec_program.asm:1`|0|0|none|
|`os/milton/overlay.ld:1`|0|0|none|
|`os/milton/overlay_hdr.asm:1`|0|0|none|
|`os/milton/ovl_fixture.asm:1`|0|0|none|
|`os/milton/ovltest_program.asm:1`|0|0|none|
|`os/milton/panic.c:1`|1|1|`os/milton/panic.c:152` `PANIC_MUTATE_SKIP_REASON`|
|`os/milton/panic.h:1`|0|0|none|
|`os/milton/panic_program.asm:1`|0|0|none|
|`os/milton/panic_screen.h:1`|2|2|`os/milton/panic_screen.h:43` `PANIC_MUTATE_OFF_CENTRE`; `os/milton/panic_screen.h:71` `PANIC_MUTATE_NO_8BPP`|
|`os/milton/partition_probe.asm:1`|0|0|none|
|`os/milton/pic.c:1`|0|0|none|
|`os/milton/pic.h:1`|0|0|none|
|`os/milton/pit.c:1`|3|2|`os/milton/pit.c:76` `PIT_MUTATE_SCRIBBLE_DOS`; `os/milton/pit.c:91` `PIT_MUTATE_SCRIBBLE_DOS`; `os/milton/pit.c:94` `PIT_MUTATE_ISSUE_INT21`|
|`os/milton/pit.h:1`|0|0|none|
|`os/milton/psp.c:1`|5|5|`os/milton/psp.c:164` `PSP_MUTATE_INT20`; `os/milton/psp.c:225` `PSP_MUT_NO_JFT_INHERIT`; `os/milton/psp.c:298` `PSP_MUTATE_NO_TAIL_CLAMP`; `os/milton/psp.c:311` `PSP_MUTATE_CMDTAIL_LEN`; `os/milton/psp.c:381` `PSP_MUTATE_VEC_OFFSET`|
|`os/milton/psp.h:1`|0|0|none|
|`os/milton/rec_sync.h:1`|0|0|none|
|`os/milton/rtc.c:1`|2|2|`os/milton/rtc.c:92` `RTC_MUTATE_SKIP_BCD`; `os/milton/rtc.c:105` `RTC_MUTATE_MONTH_OFFBYONE`|
|`os/milton/rtc.h:1`|0|0|none|
|`os/milton/sft.c:1`|4|4|`os/milton/sft.c:220` `SFT_MUTATE_DUP_NO_REFCOUNT`; `os/milton/sft.c:249` `SFT_MUTATE_DUP2_NO_RELEASE`; `os/milton/sft.c:323` `SFT_MUTATE_NO_CLOSE_PROCESS`; `os/milton/sft.c:372` `SFT_MUTATE_NO_INHERIT_REFCOUNT`|
|`os/milton/sft.h:1`|0|0|none|
|`os/milton/sysinit.c:1`|2|2|`os/milton/sysinit.c:177` `FPU_MUTATE_SKIP_INIT`; `os/milton/sysinit.c:220` `FPU_MUTATE_BOOT_CW`|
|`os/milton/sysinit.h:1`|0|0|none|
|`os/milton/sysinit_program.asm:1`|0|0|none|
|`os/milton/tsr_program.asm:1`|0|0|none|
|`os/milton/type_program.asm:1`|0|0|none|
|`os/milton/vect_program.asm:1`|0|0|none|
|`os/milton/write_program.asm:1`|0|0|none|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.

- `os/milton/block_device.h:7` → `os/milton/diskutil.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/boot_screen.h:42` → `os/flair/asset_lz.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/boot_screen.h:43` → `os/flair/asset_lz_data.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/boot_screen.h:46` → `os/flair/text.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/console.c:32` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kbd.c:18` → `os/flair/ring_report.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:39` → `os/milton/kmain_cold.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:43` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:60` → `os/milton/command.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:62` → `os/milton/power.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:65` → `os/flair/film_startup.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:77` → `os/flair/heap.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:78` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:79` → `os/flair/cursor.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:83` → `os/flair/shell.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:84` → `os/flair/shell_menus.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:85` → `os/flair/desktop.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:86` → `os/flair/flair_look.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:87` → `os/flair/chrome.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:91` → `os/flair/event.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:95` → `os/milton/desktop_db.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:102` → `os/flair/process.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:104` → `os/flair/system_key.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:105` → `os/flair/power_notice.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:106` → `os/flair/egg_jtc.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:108` → `os/flair/film_scene.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:110` → `os/apps/ref_tenant.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:112` → `os/flair/finder_desktop.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:116` → `os/flair/finder_cmd.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:119` → `os/flair/finder_windows.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:123` → `os/flair/finder_ops.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:126` → `os/flair/finder_menu.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:129` → `os/flair/tbxgate.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:131` → `os/flair/apple_menu.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:135` → `os/flair/stdfile.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:136` → `os/flair/scrap.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:137` → `os/milton/transient.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:139` → `os/milton/about_box.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:182` → `os/apps/ref_demo_menu.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:1294` → `os/milton/finder_actions.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:1295` → `os/milton/power_cold.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/loader.c:29` → `os/milton/transient.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/loader_cold.c:17` → `os/milton/transient.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/kmain.c:77` → `flair heap/window/menu/process/Finder managers`: move production desktop orchestration to flair/desktop; source slice map below.
- `os/milton/kmain.c:110` → `os/apps/ref_tenant.h`: HELLO/NOTES are resident reference tenants, not disk programs.
- `os/milton/int21.c:42` → `os/milton/ansi.c; os/milton/loader.c:50 -> mz.c`: textually folded translation units are build coupling; inspect include sites listed above.

### kmain.c split map (all 8,207 physical lines)

a = kernel; b = production desktop/Finder; c = compiled-in scenarios or test policy. Ranges are ownership guidance from source inspection, not preprocessor-executed LOC. Mutation branches are interleaved and enumerated in section 6.

|Lines|Class|Disposition / role|
|---|---|---|
|1–385|a+b|includes, shared declarations and adapters; separate by referenced subsystem; `os/milton/kmain.c:1`|
|386–995|a|DOS callbacks, process/EXEC/TSR/keyboard/idle/filesystem plumbing; CHKDSK cache adapter also serves dos; `os/milton/kmain.c:386`|
|996–1536|b|FLAIR frame, surface/palette and composed scene construction; `os/milton/kmain.c:996`|
|1537–3151|b|live cursor/input tracking, window/menu/modal/chrome and launch helpers; `os/milton/kmain.c:1537`|
|3152–5233|b|Finder bindings, filesystem provider, menus, alerts, Standard File, Apple menu/DA services; `os/milton/kmain.c:3152`|
|5234–5467|a+b+c|boot-volume provider, FLAIR.OVL loader and OVL_TEST* scenarios; loader service belongs kernel, desktop imports retire; `os/milton/kmain.c:5234`|
|5468–5745|b|Exit-to-DOS teardown, foreground activation, Finder keyboard/menu dispatch; `os/milton/kmain.c:5468`|
|5746–5936|c|TBX_HEADLESS_SMOKE_* compiled-in app/tenant driving and copy smoke keys; `os/milton/kmain.c:5746`|
|5937–6214|a|kernel entry, boot-info/placement validation, console/interrupt/sysinit/platform foundation; `os/milton/kmain.c:5937`|
|6215–6754|b+c|static/live desktop construction, HELLO/NOTES/Finder setup, input hooks; personality/mutation guards interleaved; `os/milton/kmain.c:6215`|
|6755–7268|b+c|production event pump plus tick-budget/record/interactive/test exits; ordinary image uses bounded oracle mode; `os/milton/kmain.c:6755`|
|7269–7407|c|fault/spurious and FPU_TEST/FPU_MF diagnostics; `os/milton/kmain.c:7269`|
|7408–7618|a+c|DOS filesystem setup and baked TYPE/DIR demo programs; keep mount service, remove baked demos; `os/milton/kmain.c:7408`|
|7619–8073|c|EXEC/nested/overlay/MZ/memory/write/absolute disk/multiopen/date/exit/sysinit test scenarios; `os/milton/kmain.c:7619`|
|8074–8207|a+c|shell REPL dispatch vs keyboard/IRQstorm/CONIN/VECT test personalities and terminal halt; `os/milton/kmain.c:8074`|

### Every BOOT_* personality

A program can currently execute privileged instructions because applications are flat ring 0. The recommendations below do not claim that interrupt/CR0 probes are safe ordinary DOS programs under future protection. “Special kernel” is needed only to observe failures before any loader/program can run; most current probes execute after startup.

|Selector|Condition sites|What it exercises|Replacement / special-kernel assessment|
|---|---|---|---|
|BOOT_ABSDISK|`os/milton/kmain.c:7897`|INT25/26 absolute disk guest|disk test program; yes|
|BOOT_CONIN|`os/milton/kmain.c:8140`|cooked/live console input|console test program; yes|
|BOOT_DATETIME|`os/milton/kmain.c:7936`|DOS date/time interrupt functions|disk test program; yes|
|BOOT_DOSMSG_FAULT|`os/milton/kmain.c:8084`|controlled DOS diagnostic on startup fault|test program/fault injection at service boundary; yes, with injectable device/provider|
|BOOT_EXEC|`os/milton/kmain.c:7639`|load and execute ordinary COM; register/return integrity|shell-run launcher and child programs; yes|
|BOOT_EXEC_NESTED|`os/milton/kmain.c:7619`|nested EXEC stack/PSP/handle restoration|parent and child programs on disk; yes|
|BOOT_EXEC_OVERLAY|`os/milton/kmain.c:7626`|DOS overlay/load service behavior|disk launcher + overlay fixture; yes|
|BOOT_EXITH|`os/milton/kmain.c:7951`|child exit reclaims handles|disk launcher/program; yes|
|BOOT_FLAIR_LIVE|`os/milton/kmain.c:32`, `os/milton/kmain.c:68`, `os/milton/kmain.c:830`, `os/milton/kmain.c:847`, `os/milton/kmain.c:876`, `os/milton/kmain.c:996`, `os/milton/kmain.c:1537`, `os/milton/kmain.c:3152`, `os/milton/kmain.c:6215`, `os/milton/kmain.c:6221`, `os/milton/kmain.c:6267`|live chrome/tenant desktop personality|production desktop launch mode; external traces replace bounded test builds; no special kernel; launch desktop normally|
|BOOT_FLAIR_SHELL|`os/milton/kmain.c:68`, `os/milton/kmain.c:996`, `os/milton/kmain.c:6215`, `os/milton/kmain.c:6250`|static composed desktop frame|desktop screenshot mode/external driver; no special kernel|
|BOOT_FPU_MF|`os/milton/kmain.c:7310`, `os/milton/kmain.c:7376`|native unmasked x87 exception #MF reaches panic|privileged diagnostic program; panic ends boot; yes under current ring-0 app model; isolate privileged test|
|BOOT_FPU_TEST|`os/milton/kmain.c:7310`|x87 initialization/control word/exact arithmetic|privileged diagnostic program; yes under ring-0 app model; absent-FPU variant remains emulator configuration|
|BOOT_IRQSTORM|`os/milton/kmain.c:8116`|timer IRQ continues while INT21 paths execute|disk program observing progress + external timer trace; yes; timing/EOI counters need observability service|
|BOOT_KBD_ECHO|`os/milton/kmain.c:8074`|keyboard interrupt and echo path|disk program reading CON; yes|
|BOOT_MEMTEST|`os/milton/kmain.c:7797`|MCB allocate/free/resize and expected failure|disk INT21 program; yes|
|BOOT_MULTIOPEN|`os/milton/kmain.c:7920`|independent open positions/handles|disk INT21 program; yes|
|BOOT_MZEXEC|`os/milton/kmain.c:7712`|MZ relocation and return behavior|MZ file and shell-run launcher; yes|
|BOOT_MZFOREIGN|`os/milton/kmain.c:7762`|reject foreign/invalid MZ|bad file and shell-run launcher; yes|
|BOOT_SECONDARY_SHELL|`os/milton/kmain.c:8107`|primary-slave shell/startup image choice|boot configuration or explicit mount/drive service; configuration; boot-device selection genuinely occurs before shell, no test-only kernel needed|
|BOOT_SELFTEST_FAULT|`os/milton/kmain.c:7269`|divide error panic / no triple fault|privileged faulting disk program; yes under current ring-0 model; truly early-init fault tests need diagnostic boot hook|
|BOOT_SHELL|`os/milton/kmain.c:830`, `os/milton/kmain.c:860`, `os/milton/kmain.c:7609`, `os/milton/kmain.c:8083`|COMMAND interpreter entry|production shell, not a test; no special kernel|
|BOOT_SPURIOUS|`os/milton/kmain.c:7286`|unhandled vector / IRQ7 / IRQ15 return & EOI discipline|privileged interrupt diagnostic; yes under ring-0 model; not ordinary unprivileged DOS semantics|
|BOOT_SYSINIT|`os/milton/kmain.c:8008`|CONFIG.SYS sizing, loading and device initialization|on-disk config + external boot checks; boot path must be exercised; no need to recompile kernel; early-init failure probes may require an injectable provider|
|BOOT_VECT|`os/milton/kmain.c:8157`|DOS interrupt vector services|disk INT21 program; yes|
|BOOT_WRITE|`os/milton/kmain.c:7884`|DOS create/write/flush/readback path|disk INT21 program; yes|

Non-BOOT scenario selectors also matter: `TBX_HEADLESS_SMOKE_*` around kmain:5746, `FLAIR_LIVE_TENANTS`, `FLAIR_LIVE_INTERACTIVE`, `FLAIR_LIVE_*TICKS`, `OVL_TEST_*`, recording selectors and manager mutant defines. They are not additional BOOT personalities. The default `image` is a bounded tenants oracle, while the interactive image is a separate link. Move replay inputs and completion logic outside the product.

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Gates/process wrappers to retire: `test-arena-disjoint`, `test-arena-disjoint-mutant`.
Merge `_cold.c`/`_private.h` size splits with their owning implementation, retire FLAIR.OVL resident/import stub synthesis and COMMAND transient fingerprinting as their replacements become available. Keep actual DOS overlay semantics and independently useful loader validation. Remove the 384-sector/192-KiB policing, padding-only mutants, variant-wide byte ceilings and recording/bounded personality switches from production builds.
